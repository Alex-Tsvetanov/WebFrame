import Foundation

protocol DiskCache: AnyObject {

    func getOutput(compilationItem: CompilationItem, inputData: Data) -> Data?

    func getOutput(item: String, platform: Platform?, target: OutputTarget, inputData: Data) -> Data?

    func getOutput(item: String, inputData: Data) -> Data?

    func getExpectedOutputURL(item: String) -> URL

    func getURL() -> URL

    func setOutput(compilationItem: CompilationItem, inputData: Data, outputData: Data) throws

    func setOutput(item: String, platform: Platform?, target: OutputTarget, inputData: Data, outputData: Data) throws

    func setOutput(item: String, inputData: Data, outputData: Data) throws

    func removeOutput(item: String)

}
