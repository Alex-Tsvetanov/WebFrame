//
//  File.swift
//  
//
//  Created by Carson Holgate on 4/26/22.
//

import Foundation

struct DaemonClientLog: Codable {
    var severity: String
    var log: String
}

struct DaemonSendLogsServerResponse: Codable {
    var logs: [DaemonClientLog]
}

struct DaemonErrorResponse: Codable {
    var error_message: String
}

struct DaemonClientConfigureResponse: Codable {}

struct DaemonConnectedClient: Codable {
    var client_id: Int32
    var platform: String
    var application_id: String?
}

struct DaemonClientListConnectedClientsResponse: Codable {
    var clients: [DaemonConnectedClient]
}

struct DaemonClientForwardPayloadResponse: Codable {}

struct DaemonFileManagerFileEntry: Codable {
    var filename: String
    var absolute_path: String
    var type: Int
    var modification_date: Int?
    var creation_date: Int?
}

struct DaemonFileManagerPayloadResponse: Codable {
    var data_base64: String?
    var data_string: String?
    var entries: [DaemonFileManagerFileEntry]?
}

struct DaemonServerResponse: Codable {
    var request_id: String?
    var error: DaemonErrorResponse?
    var configure: DaemonClientConfigureResponse?
    var list_connected_clients: DaemonClientListConnectedClientsResponse?
    var forward_client_payload: DaemonClientForwardPayloadResponse?
    var file_manager: DaemonFileManagerPayloadResponse?
}

struct DaemonSendLogsServerRequest: Codable {}

struct DaemonServerRequest: Codable {
    var request_id: String?
    var send_logs: DaemonSendLogsServerRequest?
}

struct DaemonResource: Codable {
    var bundle_name: String
    var file_path_within_bundle: String
    var data_base64: Data?
    var data_string: String?
}

struct DaemonUpdatedResourcesEvent: Codable {
    var resources: [DaemonResource]
}

struct DaemonPayloadFromClientEvent: Codable {
    var sender_client_id: Int32
    var payload_base64: Data?
    var payload_string: String?
}

struct DaemonServerEvent: Codable {
    var updated_resources: DaemonUpdatedResourcesEvent?
    var payload_from_client: DaemonPayloadFromClientEvent?
}

struct DaemonServerPayload: Codable {
    var request: DaemonServerRequest?
    var response: DaemonServerResponse?
    var event: DaemonServerEvent?
}

struct DaemonClientResponse: Codable {
    var request_id: String
    var error: DaemonErrorResponse?
    var send_logs: DaemonSendLogsServerResponse?
}

struct DaemonClientConfigureRequest: Codable {
    var application_id: String
    var platform: String
    var disable_hot_reload: Bool?
}

struct DaemonListConnectedClientsRequest: Codable {}

struct DaemonClientForwardPayloadRequest: Codable {
    var client_id: Int
    var payload_base64: Data?
    var payload_string: String?
}

struct DaemonClientRequest: Decodable {
    var request_id: String
    var configure: DaemonClientConfigureRequest?
    var list_connected_clients: DaemonListConnectedClientsRequest?
    var forward_client_payload: DaemonClientForwardPayloadRequest?
    var file_manager: DaemonFileManagerRequest?
}

struct DaemonJsDebuggerInfo: Codable {
    var port : Int
    var websocket_targets: [String]
}

struct DaemonClientEvent: Codable {
    var new_logs: [DaemonClientLog]?
    var js_debugger_info: DaemonJsDebuggerInfo?
}

struct DaemonFileManagerRequestBody: Decodable {
    var path: String
    var data_string: String?
    var data_base64: Data?
    var output_format: OutputFormat?

    enum OutputFormat: String, Decodable {
        case base64
        case string
    }
}

struct DaemonFileManagerRequest: Decodable {
    let type: RequestType
    let body: DaemonFileManagerRequestBody

    enum RequestType: String, Decodable {
        case write_file
        case read_file
        case read_directory
        case delete_file
    }

}

struct DaemonClientPayload: Decodable {
    var request: DaemonClientRequest?
    var response: DaemonClientResponse?
    var event: DaemonClientEvent?
}
