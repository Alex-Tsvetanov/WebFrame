# <span style="font-size: 2.25em;"> WebFrame++ ![C++](https://img.shields.io/badge/c++-%2300599C.svg?&logo=c%2B%2B&logoColor=white) ![Mocha](https://img.shields.io/badge/-mocha-%238D6748?logo=mocha&logoColor=white)<br>[![Compile & Build](https://github.com/cpp-for-everything/WebFrame/actions/workflows/ci.yaml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/compilation.yaml) [![Benchmark & Deploy](https://github.com/cpp-for-everything/WebFrame/actions/workflows/benchmark-and-deploy.yaml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/benchmark-and-deploy.yaml) [![Code Quality](https://github.com/cpp-for-everything/WebFrame/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/codeql-analysis.yml)<br>[![Issues](https://img.shields.io/github/issues/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame) [![Forks](https://img.shields.io/github/forks/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame) [![Stars](https://img.shields.io/github/stars/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame) [![License](https://img.shields.io/github/license/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame)</span>
### Make your web application faster now!
<hr>

#### Requirements
| Compiler version  | Minimum C++ standard required |
|:-----------------:|:-----------------------------:|
| GCC, Clang, MSVC  | -std=c++2a _or_ -std=c++20    |
# Testing
## Cppcheck - static code analysis
Check the static code analysis of the project [here](https://cpp-for-everything.github.io/WebFrame/main/reports/codeql_report/).
## Performance
Check the performance check of the project [here](cpp-for-everything.github.io/WebFrame/main/reports/benchmark).
## Code documentation
Check the Doxygen documentation of the library [here](https://cpp-for-everything.github.io/WebFrame/main/reports/docs/).
# How to use

1. Make sure to include the library

    ```cpp
    #include <core/core.hpp>
    ```

2. Initiate your Web application

    ```cpp
    webframe::core::application app;
    ``` 

3. Set directories for your static files

    ```cpp
    app.set_static(relative or absolute path/directory to the static files, web alias); 
    ```

    Ex. the files are in ``./example/Sample/static`` and the route for them is ``/static``:
    
    ```cpp
    app.set_static("./example/Sample/static", "/static"); 
    ```
    
    _**NOTE:** You can list multiple static folders_

    _**NOTE:** Relative paths depend on where you execute the executable from and not on where the source file is located._

4. Set Inja template folder
    
    ```cpp
    app.set_templates(relative or absolute path/directory to the INJA templates); 
    ```
    
    Ex. the Inja template files are in ``./example/Sample/static/templates``:
    
    ```cpp
    app.set_templates("./example/Sample/static/templates");
    ```
    
    _**NOTE:** You can list multiple template folders_

    _**NOTE:** Relative paths depend on where you execute the executable from and not on where the source file is located._

5. Set up error handling
    - Set the code of the error
    - Implement the responding function using lambda that takes 1 string as parameter
    
        ```cpp
        app.handle("404", [](const std::string& path) {
            return "Error 404: " + path + " was not found.";
        });
        app.handle("500", [](const std::string& reason) {
            return "Error 500: Internal server error: " + reason + ".";
        });
        ```
    
6. Set up your routes
    * Set up headers and status code yourself
    
        ```cpp
        app.route ("/", []() {
            return webframe::core::response (webframe::core::status_line ("1.1", "200"), {{"Content-Type", "text/html; charset=utf-8"}}, "<h1>Hello, World!</h1>");
        });
        ```
    * or let WebFrame do it for you
    
        ```cpp
        app.route ("/", []() {
            return "<h1>Hello, World!</h1>";
        });
        ```
    * To render your Jinja templates use ```app.render(filename/file path, json with variables)```
    * You can also use path/route variables 
        * using predefined regex
            ```cpp
            app.route("/{text}", [&app](const std::string& user) {
                return app.render("template.html", {{"username", user}});
            });
            ```
            | Predefined | Raw regex equivalent |
            |:----------:|:--------------------:|
            | string     | [A-Za-z_%0-9.-]+     |
            | text       | [A-Za-z_%0-9.-]+     |
            | char       | [A-Za-z_%0-9.-]      |
            | symbol     | [A-Za-z_%0-9.-]      |
            | digit      | [0-9]                |
            | number     | [0-9.]+              |
            | path       | [A-Za-z_%0-9.-\/]+   |
    
        * or using your own regex
            ```cpp
            app.route("/{[a-z]+}", [&app](const std::string& user) {
                return app.render("template.html", {{"username", user}});
            });
            ```
7. Make sure to run your app (async run)
    
    ```cpp
    app.run(port, cores);
    ```

## Advanced usage
1. Stack multiple setups

    You can list different app setups consequently:
    
    ```cpp
    app.set_static("./example/Sample/static", "/static")
        .set_templates("./example/Sample/static/templates")
        .handle("404", [](const std::string& path) {
            return "Error 404: " + path + " was not found.";
        })
        .handle("500", [](const std::string& reason) {
            return "Error 500: Internal server error: " + reason + ".";
        })
        .route ("/", []() {
            return webframe::core::response (webframe::core::status_line ("1.1", "200"), {{"Content-Type", "text/html; charset=utf-8"}}, "<h1>Hello, World!</h1>");
        });
    ```

2. Multithreading
    - Wait until the server starts accepting requests: (sync function)
    
        ```cpp
        app.wait_start(port);
        ```
    
        or
    
        ```cpp
        app.run(port, cores).wait_start(port);
        ```
    
    - Wait until the server stops accepting requests: (sync function)
    
        ```cpp
        app.wait_end(port);
        ```
    
        or
    
        ```cpp
        app.run(port, cores).wait_end(port);
        ```
    
3. Set up custom loggers
    - logger is the stream for standard logs from the server
    - error_logger is the stream for error messages
    - perfomancer is the stream for performance logs

        ```cpp
        app.set_logger(ostream&)
            .set_error_logger(ostream&)
            .set_performancer(ostream&);		
        ```

4. Force server stop

    ```cpp
    app.request_stop(port);
    ```

    **Note:** _port should be ``const char*``_

5. Organize projects
    - Create set of routes

        ```cpp
        init_routes(home)
            .route("/home", []() {
                return "This is my home page";
            });
        ```

        or if you need to use ``webframe::core::application`` functions

        ```cpp
        webframe::core::application app;
        ...
        init_routes(home)
            .route("/home", [&app]() {
                return app.render("template.html", {{"username", user}});
            });
        ```
    
    - Import the set of routes to your app

        ```cpp
        init_routes(home)
        ...
        app.extend_with(home);
        ```

        or if you want to add prefix to the set of routes

        ```cpp
        init_routes(home)
        ...
        app.extend_with(home, "/prefix");
        ```

Check [example/](https://github.com/cpp-for-everything/WebFrame/blob/master/example) for more information.
# Additional info
- Currently working on setting up ![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?&logo=cmake&logoColor=while)

# Socials
[![LinkedIn](https://img.shields.io/badge/linkedin-%230077B5.svg?logo=linkedin&logoColor=white)](https://www.linkedin.com/in/alex-tsvetanov/)
