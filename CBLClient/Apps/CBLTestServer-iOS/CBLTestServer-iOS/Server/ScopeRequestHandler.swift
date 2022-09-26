//
//  ScopeRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Abhay Aggrawal on 12/09/22.
//  Copyright © 2022 Raghu Sarangapani. All rights reserved.
//

import Foundation
import CouchbaseLiteSwift

public class ScopeRequestHandler {
    public func handleRequest(method: String, args: Args) throws -> Any? {
        switch method {
        case "scope_collectionObject":
            let name: String = (args.get(name: "collectionName"))!
            let scope: Scope = (args.get(name: "scope"))!
            return try scope.collection(name: name)
        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
    }
}
