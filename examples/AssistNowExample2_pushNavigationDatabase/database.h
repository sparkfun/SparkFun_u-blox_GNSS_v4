// The navigation database for AssistNowExample2_pushNavigationDatabase
//
// Copy and paste the database printed by AssistNowExample1_readNavigationDatabase here, replacing the
// databaseLen and database lines below. AssistNowExample1 prints:
//   size_t databaseLen = ...;
//   const uint8_t database[...] = { ... };
//
// On ESP32, the example reads the database from microSD (/MGADBD.UBX) first. database[databaseLen] is only
// used if the file can not be read - or if the example is not running on ESP32.

#pragma once

#include <stddef.h>
#include <stdint.h>

size_t databaseLen = 0;               // <- Replace this line
const uint8_t database[1] = { 0x00 }; // <- And this one
