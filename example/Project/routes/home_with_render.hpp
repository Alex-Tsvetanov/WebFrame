#ifndef _Project_home_with_render_route
#define _Project_home_with_render_route

#include <core/core.hpp>

webframe::core::router home_with_render(webframe::core::_application& app) {
    init_routes(home)
        .route("/home", [&app]() {
#ifdef USE_INJA
			return app.render("template.html");
#else
            return "";
#endif
        });
    return home;
}
#endif