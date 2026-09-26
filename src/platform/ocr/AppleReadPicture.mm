#include "platform/ocr/IReadPicture.hpp"

#include "core/Error.hpp"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <Vision/Vision.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::platform::ocr {
namespace {

// How sure the reader must be of a run of words before it is handed back. Below this the reader is
// guessing at shapes that are not writing.
constexpr float kSureEnough = 0.3F;

[[nodiscard]] std::string saidBy(NSString* text) {
    return text == nil ? std::string{} : std::string{[text UTF8String]};
}

}

// The words in a picture, read by the reader macOS carries. It is given the bytes of the file
// rather than a decoded picture, so that whatever the machine can open, it can read.
class AppleReadPicture final : public IReadPicture {
public:
    ~AppleReadPicture() override { giveUp(); }

    AppleReadPicture() = default;
    AppleReadPicture(const AppleReadPicture&) = delete;
    AppleReadPicture& operator=(const AppleReadPicture&) = delete;
    AppleReadPicture(AppleReadPicture&&) = delete;
    AppleReadPicture& operator=(AppleReadPicture&&) = delete;

    void read(std::span<const std::byte> picture, const std::string& language, Read read) override;

    void giveUp() override {
        if (m_asked != nil) {
            [m_asked cancel];
            m_asked = nil;
        }
    }

    [[nodiscard]] std::vector<std::string> languages() override;

private:
    VNRecognizeTextRequest* m_asked{nil};
};

void AppleReadPicture::read(std::span<const std::byte> picture, const std::string& language,
                            Read read) {
    if (picture.empty()) {
        read(core::makeError(core::ErrorCode::InvalidArgument, "there is no picture here to read"));
        return;
    }
    @autoreleasepool {
        NSData* const held = [NSData dataWithBytes:picture.data() length:picture.size()];
        CGImageSourceRef source =
            CGImageSourceCreateWithData(static_cast<CFDataRef>(held), nullptr);
        if (source == nullptr) {
            read(core::makeError(core::ErrorCode::Unsupported,
                                 "this picture is not in a form the machine can open"));
            return;
        }
        CGImageRef shown = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);
        if (shown == nullptr) {
            read(core::makeError(core::ErrorCode::Unsupported,
                                 "this picture is not in a form the machine can open"));
            return;
        }

        auto answer = std::make_shared<Read>(std::move(read));
        VNRecognizeTextRequest* const asking = [[VNRecognizeTextRequest alloc]
            initWithCompletionHandler:^(VNRequest* request, NSError* trouble) {
              if (trouble != nil) {
                  (*answer)(core::makeError(core::ErrorCode::Unknown,
                                            saidBy([trouble localizedDescription])));
                  return;
              }
              std::vector<Found> found;
              for (VNRecognizedTextObservation* seen in request.results) {
                  if (![seen isKindOfClass:[VNRecognizedTextObservation class]]
                      || seen.confidence < kSureEnough) {
                      continue;
                  }
                  NSArray<VNRecognizedText*>* const best = [seen topCandidates:1];
                  if (best.count == 0) {
                      continue;
                  }
                  const std::string said = saidBy(best.firstObject.string);
                  if (said.empty()) {
                      continue;
                  }
                  // Vision counts from the bottom left; a page counts from the top left.
                  found.push_back(Found{
                      .text = said,
                      .left = static_cast<float>(seen.boundingBox.origin.x),
                      .top = static_cast<float>(1.0 - seen.boundingBox.origin.y
                                                - seen.boundingBox.size.height),
                      .right = static_cast<float>(seen.boundingBox.origin.x
                                                  + seen.boundingBox.size.width),
                      .bottom = static_cast<float>(1.0 - seen.boundingBox.origin.y),
                  });
              }
              std::ranges::stable_sort(found, {}, &Found::top);
              (*answer)(std::move(found));
          }];
        asking.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        asking.usesLanguageCorrection = YES;
        if (!language.empty()) {
            asking.recognitionLanguages =
                @[ [NSString stringWithUTF8String:language.c_str()] ];
        }
        m_asked = asking;

        VNImageRequestHandler* const handler =
            [[VNImageRequestHandler alloc] initWithCGImage:shown options:@{}];
        NSError* trouble = nil;
        const BOOL went = [handler performRequests:@[ asking ] error:&trouble];
        CGImageRelease(shown);
        if (!went) {
            (*answer)(core::makeError(core::ErrorCode::Unknown,
                                      trouble == nil
                                          ? "the words in this picture could not be read"
                                          : saidBy([trouble localizedDescription])));
        }
        m_asked = nil;
    }
}

std::vector<std::string> AppleReadPicture::languages() {
    std::vector<std::string> said;
    @autoreleasepool {
        VNRecognizeTextRequest* const asking = [[VNRecognizeTextRequest alloc] init];
        asking.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        NSError* trouble = nil;
        NSArray<NSString*>* const known = [asking supportedRecognitionLanguagesAndReturnError:&trouble];
        if (trouble != nil) {
            return said;
        }
        said.reserve(static_cast<std::size_t>(known.count));
        for (NSString* one in known) {
            said.push_back(saidBy(one));
        }
    }
    return said;
}

std::unique_ptr<IReadPicture> openReadPicture() {
    return std::make_unique<AppleReadPicture>();
}

}
