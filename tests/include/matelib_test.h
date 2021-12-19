#ifndef MATELIB_TEST_H_INCLUDED
#define MATELIB_TEST_H_INCLUDED

#include <CUnit/Basic.h>

#include "matelib.h"

#define MATELIB_TEST_CFG_PATH "cfg/test_config.cfg"

void test_matelib_setup(void);
void test_matelib_tear_down(void);

void test_mate_init(void);

#endif
