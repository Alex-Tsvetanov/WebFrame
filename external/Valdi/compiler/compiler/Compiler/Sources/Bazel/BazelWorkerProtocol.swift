// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

struct BazelWorkerProtocol {
    // From https://github.com/bazelbuild/bazel/blob/54a547f30fd582933889b961df1d6e37a3e33d85/src/main/protobuf/worker_protocol.proto

    // An input file.
    struct Input: Codable {
        // The path in the file system where to read this input artifact from. This is
        // either a path relative to the execution root (the worker process is
        // launched with the working directory set to the execution root), or an
        // absolute path.
        let path: String

        // A hash-value of the contents. The format of the contents is unspecified and
        // the digest should be treated as an opaque token. This can be empty in some
        // cases.
        let digest: String
    }

    struct WorkRequest: Codable {

        let arguments: [String]

        // The inputs that the worker is allowed to read during execution of this
        // request.
        let inputs: [Input]?

        // Each WorkRequest must have either a unique
        // request_id or request_id = 0. If request_id is 0, this WorkRequest must be
        // processed alone (singleplex), otherwise the worker may process multiple
        // WorkRequests in parallel (multiplexing). As an exception to the above, if
        // the cancel field is true, the request_id must be the same as a previously
        // sent WorkRequest. The request_id must be attached unchanged to the
        // corresponding WorkResponse. Only one singleplex request may be sent to a
        // worker at a time.
        let requestId: Int?


        // EXPERIMENTAL: When true, this is a cancel request, indicating that a
        // previously sent WorkRequest with the same request_id should be cancelled.
        // The arguments and inputs fields must be empty and should be ignored.
        let cancel: Bool?

        // Values greater than 0 indicate that the worker may output extra debug
        // information to stderr (which will go into the worker log). Setting the
        // --worker_verbose flag for Bazel makes this flag default to 10.
        let verbosity: Int?

        // The relative directory inside the workers working directory where the
        // inputs and outputs are placed, for sandboxing purposes. For singleplex
        // workers, this is unset, as they can use their working directory as sandbox.
        // For multiplex workers, this will be set when the
        // --experimental_worker_multiplex_sandbox flag is set _and_ the execution
        // requirements for the worker includes 'supports-multiplex-sandbox'.
        // The paths in `inputs` will not contain this prefix, but the actual files
        // will be placed/must be written relative to this directory. The worker
        // implementation is responsible for resolving the file paths.
        let sandboxDir: String?
    }

    // The worker sends this message to Blaze when it finished its work on the
    // WorkRequest message.
    struct WorkResponse: Codable {
        let exitCode: Int

        // This is printed to the user after the WorkResponse has been received and is
        // supposed to contain compiler warnings / errors etc. - thus we'll use a
        // string type here, which gives us UTF-8 encoding.
        let output: String

        // This field must be set to the same request_id as the WorkRequest it is a
        // response to. Since worker processes which support multiplex worker will
        // handle multiple WorkRequests in parallel, this ID will be used to
        // determined which WorkerProxy does this WorkResponse belong to.
        let requestId: Int

        // EXPERIMENTAL When true, indicates that this response was sent due to
        // receiving a cancel request. The exit_code and output fields should be empty
        // and will be ignored. Exactly one WorkResponse must be sent for each
        // non-cancelling WorkRequest received by the worker, but if the worker
        // received a cancel request, it doesn't matter if it replies with a regular
        // WorkResponse or with one where was_cancelled = true.
        let wasCancelled: Bool
    }
}
