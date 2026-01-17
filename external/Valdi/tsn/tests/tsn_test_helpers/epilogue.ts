lastTestComplete.then(() => {
  if (failedTests.length == 0) {
    console.log(`[==========] ALL GOOD ^_^`);
  } else {
    console.log(`[==========] ${failedTests.length} test(s) failed:`);
    for (const t of failedTests) {
      console.log(`[  FAILED  ] ${t}`);
    }
    setExitCode(-1);
  }
});
