#include "app/cpp/SystemMenus.hpp"

#import <Foundation/Foundation.h>

namespace phvikapen::app {

void keepSystemItemsOutOfMenus() {
    NSUserDefaults* const defaults = [NSUserDefaults standardUserDefaults];
    [defaults setBool:YES forKey:@"NSDisabledDictationMenuItem"];
    [defaults setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];
}

}
