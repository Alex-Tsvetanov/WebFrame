//
//  SCValdiDefaultHTTPRequestManager.h
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#import <Foundation/Foundation.h>

#import "valdi_core/HTTPRequestManager.hpp"

std::shared_ptr<snap::valdi_core::HTTPRequestManager> SCValdiCreateDefaultHTTPRequestManager();
