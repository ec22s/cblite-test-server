//
//  ViewController.swift
//  CBLTestServer-iOS
//
//  Created by Raghu Sarangapani on 10/24/17.
//  Copyright © 2017 Raghu Sarangapani. All rights reserved.
//

import UIKit

class ViewController: UIViewController {
    
    var server: Server!
    var backgroundTask: UIBackgroundTaskIdentifier = UIBackgroundTaskInvalid

    override func viewDidLoad() {
        super.viewDidLoad()
        
        let label = UILabel(frame: CGRect(x: 0, y: 0, width: 360, height: 21))
        label.center = CGPoint(x: 190, y: 285)
        label.textAlignment = .center
        label.text = "CBLTestServer-iOS running @" + String(describing: getWiFiAddress()) + ":8080"
        self.view.addSubview(label)

        server = Server()
        
        
        NotificationCenter.default.addObserver(
            forName: Notification.Name.UIApplicationDidEnterBackground,
            object: nil, queue: nil) { (note) in
                self.backgroundTask = UIApplication.shared.beginBackgroundTask(
                    expirationHandler: {
                        self.backgroundTask = UIBackgroundTaskInvalid;
                })
        }
        
        NotificationCenter.default.addObserver(
            forName: Notification.Name.UIApplicationWillEnterForeground,
            object: nil, queue: nil) { (note) in
                if self.backgroundTask != UIBackgroundTaskInvalid {
                    UIApplication.shared.endBackgroundTask(self.backgroundTask)
                    self.backgroundTask = UIBackgroundTaskInvalid
                }
        }

    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

