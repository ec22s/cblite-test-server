//
//  ListenerAuthenticatorRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Manasa Ghanta on 6/12/20.
//  Copyright © 2020 Manasa Ghanta All rights reserved.
//

import Foundation
//
//  ListenerAuthenticatorRequestHandler.swift
//  CBLTestServer-iOS
//
//

import Foundation
import CouchbaseLiteSwift

public class ListenerAuthenticatorRequestHandler {
    public static let VOID: String? = nil

    func dataFromResource(name: String, ofType type: String) throws -> Data {
        let res = ("Support" as NSString).appendingPathComponent(name)
        let path = Bundle(for: Swift.type(of:self)).path(forResource: res, ofType: type)
        return try! NSData(contentsOfFile: path!, options: []) as Data
    }

    public func handleRequest(method: String, args: Args) throws -> Any? {
        switch method {
        ////////////////////////
        // Authenticator Request Handler //
        ////////////////////////
        case "listenerAuthenticator_create":
            let li_username: String = args.get(name: "username")!
            let li_password: String = args.get(name: "password")!
            let listenerAuth = ListenerPasswordAuthenticator {
                (username, password) -> Bool in
                return (username as NSString).isEqual(to: li_username) &&
                       (password as NSString).isEqual(to: li_password)
            }
            return listenerAuth
        #if COUCHBASE_ENTERPRISE
        case "listenerAuth_listenerCertificateAuthenticator_create":
            let rootCertData = try dataFromResource(name: "identity2/client-ca", ofType: "der")
            let rootCert = SecCertificateCreateWithData(kCFAllocatorDefault, rootCertData as CFData)!

            let listenerAuth = ListenerCertificateAuthenticator.init(rootCerts: [rootCert])
            return listenerAuth
        #endif
        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
    }
}
