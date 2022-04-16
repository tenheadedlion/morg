#pragma once
// Organizer
// ===========================
//
//
// Architecture: Use 2 queues to coordinate work flow
//
//                               Worker1
//                               Worker2
//                               Worker3
// TaskSpawner----->Queue1-----> Worker4
//                    ^             |
//                    |             |
//    +----------------             |
//    |                             |
//    |                             |
//  Relay<------ Queue2<------------+
//

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif
#include <morg/types.h>
#include <morg/parser.h>
#include <morg/driver.h>


