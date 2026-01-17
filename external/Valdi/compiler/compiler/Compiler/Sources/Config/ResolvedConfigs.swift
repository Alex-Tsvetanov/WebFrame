import Foundation

struct ResolvedConfigs {
    let compilerConfig: CompilerConfig
    let projectConfig: ValdiProjectConfig
    let userConfig: ValdiUserConfig

    static func from(logger: ILogger, baseURL: URL, userConfigURL: URL, args: ValdiCompilerArguments) throws -> ResolvedConfigs {
        var userConfig = ValdiUserConfig(usernames: [], deviceIds: [], ipAddresses: [], logsOutputDir: nil)
        if FileManager.default.fileExists(atPath: userConfigURL.path) {
            userConfig = try ValdiUserConfig.from(configUrl: userConfigURL, args: args)
            logger.info("Parsed the user config at \(userConfigURL.path)")
        }

        guard let projectConfigPath = args.config else {
            throw CompilerError("No project config provided")
        }

        var environment = ProcessInfo.processInfo.environment
        // Insert PWD magic env var if its not there
        environment["PWD"] = baseURL.path

        let projectConfigURL = baseURL.resolving(path: projectConfigPath, isDirectory: false)
        let projectConfig = try ValdiProjectConfig.from(logger: logger, configUrl: projectConfigURL, currentDirectoryUrl: baseURL, environment: environment, args: args)
        logger.info("Parsed the project config at \(projectConfigURL.path)")

        let compilerConfig = try CompilerConfig.from(args: args, baseURL: baseURL, environment: environment)
        return ResolvedConfigs(compilerConfig: compilerConfig,
                               projectConfig: projectConfig,
                               userConfig: userConfig)
    }
}
