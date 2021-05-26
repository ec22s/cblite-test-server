//
//  ViewController.swift
//  TestServer
//
//  Created by Jim Borden on 5/18/21.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        
        CBLTestServerBridge.startTestServer();
    }


}

