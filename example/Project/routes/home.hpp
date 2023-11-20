#ifndef _Project_home_route
#define _Project_home_route

#include <core/core.hpp>

init_routes(home)
    .route("/home", []() {
        return "This is my home page";
    });

#endif