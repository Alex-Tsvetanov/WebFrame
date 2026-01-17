//
//  SCValdiDefaultHTTPRequestManager.m
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#import "SCValdiDefaultHTTPRequestManager.h"
#import "valdi_core/HTTPRequest.hpp"
#import "valdi_core/HTTPRequestManagerCompletion.hpp"
#import "valdi_core/HTTPResponse.hpp"
#import "valdi_core/Cancelable.hpp"
#import "SCValdiObjCUtils.h"

class HTTPRequestTask: public snap::valdi_core::Cancelable {
public:
    HTTPRequestTask(NSURLSessionTask *task): _task(task) {

    }

    ~HTTPRequestTask() override {

    }

    void cancel() override {
        [_task cancel];
    }

private:
    NSURLSessionTask *_task;
};

class HTTPRequestManager: public snap::valdi_core::HTTPRequestManager {
public:
    std::shared_ptr<snap::valdi_core::Cancelable> performRequest(const snap::valdi_core::HTTPRequest & request, const std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion> & completion) override {
        NSString *urlString = NSStringFromString(request.url);

        NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString:urlString]];
        urlRequest.HTTPMethod = NSStringFromString(request.method);

        if (request.body.has_value()) {
            urlRequest.HTTPBody = [NSData dataWithBytes:request.body.value().data() length:request.body.value().size()];
        }

        if (request.headers.isMap()) {
            for (const auto &it: *request.headers.getMap()) {
                NSString *key = NSStringFromString(it.first);
                NSString *value = NSStringFromString(it.second.toStringBox());
                [urlRequest setValue:value forHTTPHeaderField:key];
            }
        }

        __block std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion> capturedCompletion = completion;

        NSURLSessionTask *task = [NSURLSession.sharedSession dataTaskWithRequest:urlRequest
                                                               completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            if (error) {
                if (error.code == NSURLErrorCancelled) {
                    return;
                }

                auto str = StringFromNSString(error.localizedDescription);
                capturedCompletion->onFail(str.slowToString());

            } else {
                NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse *)response;

                NSDictionary *allHeaderFields = urlResponse.allHeaderFields;

                Valdi::Value headers;
                for (NSString *key in allHeaderFields.allKeys) {
                    NSString *value = allHeaderFields[key];

                    headers.setMapValue(StringFromNSString(key), Valdi::Value(StringFromNSString(value)));
                }

                std::optional<Valdi::BytesView> body;
                if (data) {
                    body = {Valdi::BytesView(ValdiObjectFromNSObject(data, YES), reinterpret_cast<const Valdi::Byte *>(data.bytes),
                                   static_cast<size_t>(data.length))};
                }

                capturedCompletion->onComplete(snap::valdi_core::HTTPResponse((int32_t)urlResponse.statusCode, headers, body));
            }
        }];

        [task resume];

        return std::make_shared<HTTPRequestTask>(task);
    }
};

std::shared_ptr<snap::valdi_core::HTTPRequestManager> SCValdiCreateDefaultHTTPRequestManager() {
    return std::make_shared<HTTPRequestManager>();
}

