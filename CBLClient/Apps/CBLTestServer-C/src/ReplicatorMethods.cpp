#include "ReplicatorMethods.h"
#include "MemoryMap.h"
#include "Router.h"
#include "Defines.h"

#include INCLUDE_CBL(CouchbaseLite.h)
#include <sstream>
#include <vector>
using namespace nlohmann;
using namespace std;

const char* const kCBLErrorDomainNames[7] = {
    nullptr,
    "CouchbaseLite",
    "POSIX",
    "SQLite",
    "Fleece",
    "Network",
    "WebSocket"
};

static void CBLReplicator_EntryDelete(void* ptr) {
    CBLReplicator_Release(static_cast<CBLReplicator *>(ptr));
}

class ReplicationChangeListenerProxy {
public:
    void handleChange(const CBLReplicatorStatus* change) {
        _changes.push_back(*change);
    }

    void registerListener(CBLReplicator* repl) {
        if(_token) {
            throw domain_error("Change listener already registered");
        }

        _token = CBLReplicator_AddChangeListener(repl, CBLReplicatorChangeCallback, this);
    }

    void unregisterListener() {
        if(_token) {
            CBLListener_Remove(_token);
            _token = nullptr;
        }
    }

    const vector<CBLReplicatorStatus>& changes() { return _changes; }

private:
    vector<CBLReplicatorStatus> _changes;
    CBLListenerToken* _token {nullptr};

    static void CBLReplicatorChangeCallback(void* context, CBLReplicator* r, const CBLReplicatorStatus* status) {
        auto* proxy = (ReplicationChangeListenerProxy *)context;
        proxy->handleChange(status);
    }
};

static void ReplicationChangeListenerProxy_EntryDelete(void* ptr) {
    delete (ReplicationChangeListenerProxy *)ptr;
}

class EventReplicationListenerProxy {
public:
    struct ReplicatedDocument {
        ReplicatedDocument(FLString docID, CBLDocumentFlags flags, CBLError err) 
            :docID(to_string(docID))
            ,flags(flags)
            ,err(err)
        {}

        string docID;
        CBLDocumentFlags flags;
        CBLError err;
    };

    struct EventArgs {
        bool isPush;
        vector<ReplicatedDocument> docs;
    };

    void handleChange(bool isPush, unsigned numDocs, const CBLReplicatedDocument* docs) {
        EventArgs args {
            isPush
        };

        for(unsigned i = 0; i < numDocs; i++) {
            auto doc = docs[i];
            args.docs.emplace_back(doc.ID, doc.flags, doc.error);
        }

        _changes.push_back(args);
    }

    void registerListener(CBLReplicator* repl) {
        if(_token) {
            throw domain_error("Change listener already registered");
        }

        _token = CBLReplicator_AddDocumentReplicationListener(repl, CBLReplicatorEventCallback, this);
    }

    void unregisterListener() {
        if(_token) {
            CBLListener_Remove(_token);
            _token = nullptr;
        }
    }

    const vector<EventArgs>& changes() { return _changes; }
private:
    vector<EventArgs> _changes;
    CBLListenerToken* _token;

    static void CBLReplicatorEventCallback(void* context, CBLReplicator* r, bool isPush, unsigned c, const CBLReplicatedDocument* docs) {
        auto* proxy = (EventReplicationListenerProxy *)context;
        proxy->handleChange(isPush, c, docs);
    }
};

static void EventReplicationListenerProxy_EntryDelete(void* ptr) {
    delete (EventReplicationListenerProxy *)ptr;
}

namespace replicator_methods {
    void replicator_create(json& body, mg_connection* conn) {
        with<CBLReplicatorConfiguration *>(body, "config", [conn](CBLReplicatorConfiguration* repConf)
        {
            CBLError err;
            CBLReplicator* repl;
            TRY((repl = CBLReplicator_Create(repConf, &err)), err)
            write_serialized_body(conn, memory_map::store(repl, CBLReplicator_EntryDelete));
        });
    }

    void replicator_start(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [](CBLReplicator* r)
        {
            CBLReplicator_Start(r, false);
        });

        mg_send_http_ok(conn, "application/text", 0);
    }

    void replicator_stop(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [](CBLReplicator* r)
        {
            CBLReplicator_Stop(r);
        });

        mg_send_http_ok(conn, "application/text", 0);
    }

    void replicator_status(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r)
        {
            json body;
            json progress;
            json error;
            const auto status = CBLReplicator_Status(r);
            body["activityLevel"] = status.activity;
            progress["documentCount"] = status.progress.documentCount;
            progress["complete"] = status.progress.complete;
            body["progress"] = progress;
            error["domain"] = status.error.domain;
            error["code"] = status.error.code;
            auto errMsg = CBLError_Message(&status.error);
            error["message"] = to_string(errMsg);
            body["error"] = error;
            
            write_serialized_body(conn, body.dump());

            FLSliceResult_Release(errMsg);
        });
    }

    void replicator_getActivityLevel(json& body, mg_connection* conn) {
        static const char* activityLevelNames[] = {
            "stopped", "offline", "connecting", "busy", "idle"
        };

        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r)
        {
            const auto status = CBLReplicator_Status(r);
            write_serialized_body(conn, activityLevelNames[status.activity]);
        });
    }

    void replicator_getError(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r)
        {
            const auto status = CBLReplicator_Status(r);
            if((int)status.error.domain == 0 && status.error.code == 0) {
                mg_send_http_ok(conn, "application/text", 0);
                return;
            }

            stringstream ss;
            string errMsg = to_string(CBLError_Message(&status.error));
            ss << "Error " << status.error.domain << " / " << status.error.code << ": " << errMsg;
            write_serialized_body(conn, ss.str());
        });
    }

    void replicator_getTotal(json& body, mg_connection* conn) {
        // Note: Due to the new API not having the total count, return 1.0
        // in place of the actual total since having completed == 1.0 means
        // it is done
        write_serialized_body(conn, 1.0f);
    }

    void replicator_getCompleted(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r) 
        {
            auto status = CBLReplicator_Status(r);
            if(abs(status.progress.complete - 1.0f) - numeric_limits<float>::epsilon() <= 0) {
                // Send the exact bytes just in case the client is using ==
                write_serialized_body(conn, 1.0f);
            } else {
                write_serialized_body(conn, status.progress.complete);
            }
        });
    }

    void replicator_addChangeListener(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r)
        {
            auto* proxy = new ReplicationChangeListenerProxy();
            proxy->registerListener(r);
            write_serialized_body(conn, memory_map::store(proxy, ReplicationChangeListenerProxy_EntryDelete));
        });
    }

    void replicator_addReplicatorEventChangeListener(json& body, mg_connection* conn) {
        with<CBLReplicator *>(body, "replicator", [conn](CBLReplicator* r)
        {
            auto* proxy = new EventReplicationListenerProxy();
            proxy->registerListener(r);
            write_serialized_body(conn, memory_map::store(proxy, EventReplicationListenerProxy_EntryDelete));
        });
    }

    void replicator_removeReplicatorEventListener(json& body, mg_connection* conn) {
        with<EventReplicationListenerProxy *>(body, "changeListener", [](EventReplicationListenerProxy *listener)
        {
            listener->unregisterListener();
        });

        mg_send_http_ok(conn, "application/text", 0);
    }

    void replicator_removeChangeListener(json& body, mg_connection* conn) {
        with<ReplicationChangeListenerProxy *>(body, "changeListener", [](ReplicationChangeListenerProxy *listener)
        {
            listener->unregisterListener();
        });

        mg_send_http_ok(conn, "application/text", 0);
    }

    void replicator_replicatorEventGetChanges(json& body, mg_connection* conn) {
        with<EventReplicationListenerProxy *>(body, "changeListener", [conn](EventReplicationListenerProxy *listener)
        {
            json eventList(0, nullptr);
            for(const auto& change : listener->changes()) {
                for(const auto& doc : change.docs) {
                    // This makes me sad :(
                    stringstream ss;
                    ss << "doc_id: " << doc.docID;
                    int error_code = 0;
                    string error_domain = "0";
                    if((int)doc.err.domain != 0) {
                        error_domain = kCBLErrorDomainNames[(int)doc.err.domain];
                        error_code = doc.err.code;
                    }

                    ss << ", error_code: " << error_code << ", error_domain: " << error_domain << ", flags: " << doc.flags
                        << ", push: " << (change.isPush ? "true"  : "false");
                    eventList.push_back(ss.str());
                }
            }

            write_serialized_body(conn, eventList);
        });
    }

    void replicator_config(json& body, mg_connection* conn) {
        auto repl = (CBLReplicator *)memory_map::get(body["replicator"].get<string>());
        auto retVal = memory_map::store(CBLReplicator_Config(repl), nullptr);
        write_serialized_body(conn, retVal);
    }

    void replicator_replicatorEventChangesCount(json& body, mg_connection* conn) {
        with<EventReplicationListenerProxy *>(body, "changeListener", [conn](EventReplicationListenerProxy *listener)
        {
            write_serialized_body(conn, listener->changes().size());
        });
    }

    void replicator_changeListenerChangesCount(json& body, mg_connection* conn) {
        with<ReplicationChangeListenerProxy *>(body, "changeListener", [conn](ReplicationChangeListenerProxy *listener)
        {
            write_serialized_body(conn, listener->changes().size());
        });
    }
}
