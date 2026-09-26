#include "platform/speech/ISpeech.hpp"

#include "core/Error.hpp"
#include "core/model/Recording.hpp"

#import <Foundation/Foundation.h>
#import <Speech/Speech.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::platform::speech {
namespace {

constexpr std::int64_t kThousand = 1000;

[[nodiscard]] std::string saidBy(NSString* text) {
    return text == nil ? std::string{} : std::string{[text UTF8String]};
}

[[nodiscard]] std::int64_t atMoment(NSTimeInterval seconds) {
    return static_cast<std::int64_t>(std::llround(seconds * static_cast<double>(kThousand)));
}

// What was heard, one run per word as the reader gives it, gathered into lines that can be read.
[[nodiscard]] std::vector<core::Saying> heardIn(SFTranscription* said) {
    if (said == nil) {
        return {};
    }
    std::vector<core::Saying> words;
    for (SFTranscriptionSegment* word in said.segments) {
        const std::string spoken = saidBy(word.substring);
        if (spoken.empty()) {
            continue;
        }
        words.push_back(core::Saying{
            .from = atMoment(word.timestamp),
            .to = atMoment(word.timestamp + word.duration),
            .text = spoken,
        });
    }
    std::vector<core::Saying> lines = core::linesOf(words);
    if (lines.empty()) {
        const std::string whole = saidBy(said.formattedString);
        if (!whole.empty()) {
            lines.push_back(core::Saying{.from = 0, .to = 0, .text = whole});
        }
    }
    return lines;
}

[[nodiscard]] NSLocale* spokenIn(const std::string& language) {
    if (language.empty()) {
        return [NSLocale currentLocale];
    }
    return [NSLocale localeWithLocaleIdentifier:[NSString stringWithUTF8String:language.c_str()]];
}

// The reader is only ever asked for where the application can say why it is asking. Without that
// line in the bundle the machine stops the whole application rather than the reading.
[[nodiscard]] bool mayAsk() {
    return [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSSpeechRecognitionUsageDescription"]
           != nil;
}

[[nodiscard]] NSURL* writtenOut(std::span<const std::byte> sound) {
    NSString* const named = [NSString stringWithFormat:@"%@.m4a", [[NSUUID UUID] UUIDString]];
    NSURL* const file =
        [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:named]];
    NSData* const held = [NSData dataWithBytes:sound.data() length:sound.size()];
    return [held writeToURL:file atomically:YES] ? file : nil;
}

void forget(NSURL* file) {
    if (file != nil) {
        [[NSFileManager defaultManager] removeItemAtURL:file error:nil];
    }
}

}

// Reading a recording back as words, by the reader macOS carries. It is asked for the whole
// recording at once rather than as it is made, so that a recording made long ago reads the same as
// one made a moment ago.
class AppleSpeech final : public ISpeech {
public:
    ~AppleSpeech() override {
        *m_here = false;
        giveUp();
    }

    AppleSpeech() = default;
    AppleSpeech(const AppleSpeech&) = delete;
    AppleSpeech& operator=(const AppleSpeech&) = delete;
    AppleSpeech(AppleSpeech&&) = delete;
    AppleSpeech& operator=(AppleSpeech&&) = delete;

    void read(std::span<const std::byte> sound, const std::string& language, Heard heard) override;

    void giveUp() override {
        if (m_asked != nil) {
            [m_asked cancel];
            m_asked = nil;
        }
    }

    [[nodiscard]] std::vector<std::string> languages() override;

private:
    using Answer = std::shared_ptr<Heard>;

    void listen(NSURL* file, const std::string& language, const Answer& answer);
    void askThenListen(NSURL* file, const std::string& language, const Answer& answer);

    SFSpeechRecognitionTask* m_asked{nil};
    std::shared_ptr<bool> m_here{std::make_shared<bool>(true)};
};

void AppleSpeech::read(std::span<const std::byte> sound, const std::string& language, Heard heard) {
    const auto answer = std::make_shared<Heard>(std::move(heard));
    if (sound.empty()) {
        (*answer)(
            core::makeError(core::ErrorCode::InvalidArgument, "there is nothing here to read"));
        return;
    }
    if (!mayAsk()) {
        (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                  "this build cannot ask to use speech recognition"));
        return;
    }
    @autoreleasepool {
        NSURL* const file = writtenOut(sound);
        if (file == nil) {
            (*answer)(core::makeError(core::ErrorCode::IoFailure,
                                      "the recording could not be handed to the reader"));
            return;
        }
        if ([SFSpeechRecognizer authorizationStatus]
            == SFSpeechRecognizerAuthorizationStatusAuthorized) {
            listen(file, language, answer);
            return;
        }
        askThenListen(file, language, answer);
    }
}

void AppleSpeech::askThenListen(NSURL* file, const std::string& language, const Answer& answer) {
    if ([SFSpeechRecognizer authorizationStatus]
        != SFSpeechRecognizerAuthorizationStatusNotDetermined) {
        forget(file);
        (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                  "this machine is not allowed to read speech; it can be allowed "
                                  "again under Privacy in System Settings"));
        return;
    }
    const std::shared_ptr<bool> here = m_here;
    // Held by a pointer the block can take along without any chance of failing.
    const auto spoken = std::make_shared<const std::string>(language);
    AppleSpeech* const reader = this;
    [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus how) {
      dispatch_async(dispatch_get_main_queue(), ^{
        if (!*here) {
            forget(file);
            return;
        }
        if (how != SFSpeechRecognizerAuthorizationStatusAuthorized) {
            forget(file);
            (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                      "this machine is not allowed to read speech"));
            return;
        }
        reader->listen(file, *spoken, answer);
      });
    }];
}

void AppleSpeech::listen(NSURL* file, const std::string& language, const Answer& answer) {
    SFSpeechRecognizer* const reader = [[SFSpeechRecognizer alloc] initWithLocale:spokenIn(language)];
    if (reader == nil || !reader.isAvailable) {
        forget(file);
        (*answer)(core::makeError(core::ErrorCode::Unsupported,
                                  "this machine cannot read speech in that language"));
        return;
    }
    SFSpeechURLRecognitionRequest* const asking =
        [[SFSpeechURLRecognitionRequest alloc] initWithURL:file];
    asking.shouldReportPartialResults = NO;
    asking.taskHint = SFSpeechRecognitionTaskHintDictation;
    if (reader.supportsOnDeviceRecognition) {
        asking.requiresOnDeviceRecognition = YES;
    }

    const std::shared_ptr<bool> here = m_here;
    AppleSpeech* const held = this;
    m_asked = [reader recognitionTaskWithRequest:asking
                                   resultHandler:^(SFSpeechRecognitionResult* got, NSError* trouble) {
                                     if (trouble == nil && (got == nil || !got.isFinal)) {
                                         return;
                                     }
                                     std::vector<core::Saying> lines;
                                     if (trouble == nil) {
                                         lines = heardIn(got.bestTranscription);
                                     }
                                     const std::string why =
                                         trouble == nil ? std::string{}
                                                        : saidBy([trouble localizedDescription]);
                                     dispatch_async(dispatch_get_main_queue(), ^{
                                       forget(file);
                                       if (!*here) {
                                           return;
                                       }
                                       held->m_asked = nil;
                                       if (!why.empty()) {
                                           (*answer)(
                                               core::makeError(core::ErrorCode::Unknown, why));
                                           return;
                                       }
                                       (*answer)(lines);
                                     });
                                   }];
    if (m_asked == nil) {
        forget(file);
        (*answer)(
            core::makeError(core::ErrorCode::Unknown, "the recording could not be read"));
    }
}

std::vector<std::string> AppleSpeech::languages() {
    std::vector<std::string> said;
    @autoreleasepool {
        NSSet<NSLocale*>* const known = [SFSpeechRecognizer supportedLocales];
        said.reserve(static_cast<std::size_t>(known.count));
        for (NSLocale* one in known) {
            const std::string name = saidBy(one.localeIdentifier);
            if (!name.empty()) {
                said.push_back(name);
            }
        }
    }
    std::ranges::sort(said);
    return said;
}

std::unique_ptr<ISpeech> openSpeech() {
    return std::make_unique<AppleSpeech>();
}

}
