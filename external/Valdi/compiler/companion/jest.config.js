// jest.config.js
//Sync object
module.exports = {
  //verbose: true,
  preset: 'ts-jest',
  testEnvironment: 'node',
  testPathIgnorePatterns: ['remotedebug-ios-webkit-adapter'],
  modulePathIgnorePatterns: ['remotedebug-ios-webkit-adapter'],
};
