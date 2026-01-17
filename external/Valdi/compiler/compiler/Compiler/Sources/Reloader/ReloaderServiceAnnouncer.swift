//
//  ReloaderServiceAnnouncer.swift
//  Compiler
//
//  Created by saniul on 01/05/2019.
//

import Foundation

protocol ReloaderServiceAnnouncerType {
}

final class ReloaderServiceAnnouncer {
    private let logger: ILogger
    private let processID = UUID().uuidString
    private let subannouncers: [ReloaderServiceAnnouncerType]

    init(logger: ILogger, deviceWhitelist: ReloaderDeviceWhitelist, projectConfig: ValdiProjectConfig, servicePort: Int) throws {
        self.logger = logger
        var subannouncers: [ReloaderServiceAnnouncerType] = []
        do {
            subannouncers.append(try ReloaderServiceUDPAnnouncer(logger: logger, processID: processID, deviceWhitelist: deviceWhitelist, servicePort: servicePort))
        } catch {
            logger.error("Failed to setup UDP reloader: \(error.legibleLocalizedDescription)")
        }

        self.subannouncers = subannouncers
    }
}
