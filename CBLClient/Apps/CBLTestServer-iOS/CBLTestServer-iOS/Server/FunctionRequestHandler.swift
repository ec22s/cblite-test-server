//
//  FunctionRequestHandler.swift
//  CBLTestServer-iOS
//
//  Created by Raghu Sarangapani on 1/16/18.
//  Copyright © 2018 Raghu Sarangapani. All rights reserved.
//

import Foundation
import CouchbaseLiteSwift

public class FunctionRequestHandler {
    public static let VOID: String? = nil
    fileprivate var _pushPullReplListener:NSObjectProtocol?
    
    public func handleRequest(method: String, args: Args) throws -> Any? {
        switch method {
        //////////////
        // Function //
        //////////////
        case "function_avg":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.avg(expression as! ExpressionProtocol)
            
        case "function_count":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.count(expression as! ExpressionProtocol)
            
        case "function_min":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.min(expression as! ExpressionProtocol)
            
        case "function_max":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.max(expression as! ExpressionProtocol)
            
        case "function_sum":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.sum(expression as! ExpressionProtocol)
            
        case "function_abs":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.abs(expression as! ExpressionProtocol)
            
        case "function_acos":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.acos(expression as! ExpressionProtocol)
            
        case "function_asin":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.asin(expression as! ExpressionProtocol)
            
        case "function_atan":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.atan(expression as! ExpressionProtocol)
            
        case "function_atan2":
            let x: Float = args.get(name: "x")!
            let y: Float = args.get(name: "y")!
            
            return Function.atan2(y:Expression.float(y), x:Expression.float(x))
            
        case "function_ceil":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.ceil(expression as! ExpressionProtocol)
            
        case "function_cos":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.cos(expression as! ExpressionProtocol)
            
        case "function_degrees":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.degrees(expression as! ExpressionProtocol)
            
        case "function_e":
            return Function.e()
            
        case "function_exp":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.exp(expression as! ExpressionProtocol)
            
        case "function_floor":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.floor(expression as! ExpressionProtocol)
            
        case "function_ln":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.ln(expression as! ExpressionProtocol)
            
        case "function_log":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.log(expression as! ExpressionProtocol)
            
        case "function_pi":
            return Function.pi()
            
        case "function_power":
            let base: Int = args.get(name: "base")!
            let exponent: Int = args.get(name: "exponent")!
            
            return Function.power(base: CouchbaseLiteSwift.Expression.int(base), exponent:CouchbaseLiteSwift.Expression.int(exponent))
            
        case "function_radians":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.radians(expression as! ExpressionProtocol)
            
        case "function_round":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.round(expression as! ExpressionProtocol)
            
        case "function_roundDigits":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            let digits: Int = args.get(name: "digits")!
            
            return Function.round(expression as! ExpressionProtocol, digits: CouchbaseLiteSwift.Expression.int(digits))
            
        case "function_sign":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.sign(expression as! ExpressionProtocol)
            
        case "function_sin":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.sin(expression as! ExpressionProtocol)
            
        case "function_sqrt":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.sqrt(expression as! ExpressionProtocol)
            
        case "function_tan":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.tan(expression as! ExpressionProtocol)
            
        case "function_trunc":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.trunc(expression as! ExpressionProtocol)
            
        case "function_truncDigits":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            let digits: Int = args.get(name: "digits")!
            
            return Function.trunc(expression as! ExpressionProtocol, digits: Expression.int(digits))
            
        case "function_contains":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            let substring: CouchbaseLiteSwift.Expression = args.get(name: "substring")!
            return Function.contains(expression as! ExpressionProtocol, substring: substring as! ExpressionProtocol)
            
        case "function_length":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.length(expression as! ExpressionProtocol)
            
        case "function_lower":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.lower(expression as! ExpressionProtocol)
            
        case "function_ltrim":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.ltrim(expression as! ExpressionProtocol)
            
        case "function_rtrim":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.rtrim(expression as! ExpressionProtocol)
            
        case "function_trim":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.trim(expression as! ExpressionProtocol)
            
        case "function_upper":
            let expression: CouchbaseLiteSwift.Expression = args.get(name: "expression")!
            return Function.upper(expression as! ExpressionProtocol)
            
        case "function_rank":
            let indexName: String = args.get(name: "expression")!
            return FullTextFunction.rank(indexName)

        default:
            throw RequestHandlerError.MethodNotFound(method)
        }
    }
}
