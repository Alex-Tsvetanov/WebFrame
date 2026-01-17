#include "valdi/cli_runner/CLIRunner.hpp"

int main(int argc, const char** argv) {
    return Valdi::valdiCLIRun("@VALDI_SCRIPT_PATH@", argc, argv);
}
