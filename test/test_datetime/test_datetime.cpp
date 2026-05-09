#include <unity.h>
#include "src/datetime.h"

void setUp(void) {}
void tearDown(void) {}

// daysInMonth -----------------------------------------------------------------

void test_daysInMonth_regular(void) {
  TEST_ASSERT_EQUAL_UINT8(31, daysInMonth(2024, 1));
  TEST_ASSERT_EQUAL_UINT8(30, daysInMonth(2024, 4));
  TEST_ASSERT_EQUAL_UINT8(31, daysInMonth(2024, 7));
  TEST_ASSERT_EQUAL_UINT8(31, daysInMonth(2024, 12));
}

void test_daysInMonth_february_leap(void) {
  TEST_ASSERT_EQUAL_UINT8(29, daysInMonth(2024, 2));
  TEST_ASSERT_EQUAL_UINT8(29, daysInMonth(2000, 2)); // century divisible by 400
}

void test_daysInMonth_february_nonleap(void) {
  TEST_ASSERT_EQUAL_UINT8(28, daysInMonth(2023, 2));
  TEST_ASSERT_EQUAL_UINT8(28, daysInMonth(1900, 2)); // century not divisible by 400
}

// daysInYear ------------------------------------------------------------------

void test_daysInYear(void) {
  TEST_ASSERT_EQUAL_INT(366, daysInYear(2024));
  TEST_ASSERT_EQUAL_INT(365, daysInYear(2023));
  TEST_ASSERT_EQUAL_INT(366, daysInYear(2000));
  TEST_ASSERT_EQUAL_INT(365, daysInYear(1900));
}

// dayOfWeek (0=Sunday, 1=Monday, ..., 6=Saturday) -----------------------------

void test_dayOfWeek_known_dates(void) {
  TEST_ASSERT_EQUAL_UINT8(1, dayOfWeek(2024, 1, 1));   // Monday
  TEST_ASSERT_EQUAL_UINT8(6, dayOfWeek(2000, 1, 1));   // Saturday (Y2K)
  TEST_ASSERT_EQUAL_UINT8(5, dayOfWeek(2026, 5, 8));   // Friday
  TEST_ASSERT_EQUAL_UINT8(0, dayOfWeek(2024, 3, 3));   // Sunday
}

void test_dayOfWeek_january_february_edge(void) {
  // Zeller treats Jan/Feb as months 13/14 of previous year — make sure year
  // boundary is handled correctly.
  TEST_ASSERT_EQUAL_UINT8(0, dayOfWeek(2023, 1, 1));   // Sunday
  TEST_ASSERT_EQUAL_UINT8(3, dayOfWeek(2020, 2, 26));  // Wednesday
}

// nthSunday -------------------------------------------------------------------

void test_nthSunday_positive(void) {
  // March 2024: Sundays on 3, 10, 17, 24, 31
  TEST_ASSERT_EQUAL_UINT8(3,  nthSunday(2024, 3, 1));
  TEST_ASSERT_EQUAL_UINT8(10, nthSunday(2024, 3, 2));
  TEST_ASSERT_EQUAL_UINT8(17, nthSunday(2024, 3, 3));
  TEST_ASSERT_EQUAL_UINT8(31, nthSunday(2024, 3, 5));
}

void test_nthSunday_negative(void) {
  // November 2024: Sundays on 3, 10, 17, 24
  TEST_ASSERT_EQUAL_UINT8(24, nthSunday(2024, 11, -1));
  TEST_ASSERT_EQUAL_UINT8(17, nthSunday(2024, 11, -2));
}

// US DST rules: 2024 transitions Mar 10 (2nd Sun Mar) and Nov 3 (1st Sun Nov).
void test_nthSunday_us_dst_rules(void) {
  TEST_ASSERT_EQUAL_UINT8(10, nthSunday(2024, 3, 2));
  TEST_ASSERT_EQUAL_UINT8(3,  nthSunday(2024, 11, 1));
}

// dateToDayCount --------------------------------------------------------------

void test_dateToDayCount_year_start(void) {
  TEST_ASSERT_EQUAL_INT(0, dateToDayCount(2024, 1, 1));
}

void test_dateToDayCount_progression(void) {
  TEST_ASSERT_EQUAL_INT(31, dateToDayCount(2024, 2, 1));   // after Jan
  TEST_ASSERT_EQUAL_INT(60, dateToDayCount(2024, 3, 1));   // after Jan+Feb (29)
  TEST_ASSERT_EQUAL_INT(59, dateToDayCount(2023, 3, 1));   // non-leap
  TEST_ASSERT_EQUAL_INT(365, dateToDayCount(2024, 12, 31));// end of leap year
}

// isTimeInRange ---------------------------------------------------------------

void test_isTimeInRange_normal(void) {
  TEST_ASSERT_TRUE (isTimeInRange(60, 120, 90));
  TEST_ASSERT_FALSE(isTimeInRange(60, 120, 50));
  TEST_ASSERT_FALSE(isTimeInRange(60, 120, 130));
  TEST_ASSERT_TRUE (isTimeInRange(60, 120, 60));   // start inclusive
  TEST_ASSERT_FALSE(isTimeInRange(60, 120, 120));  // end exclusive
}

void test_isTimeInRange_midnight_cross(void) {
  // 22:00–06:00 wrap (1320 to 360)
  TEST_ASSERT_TRUE (isTimeInRange(1320, 360, 1380)); // 23:00
  TEST_ASSERT_TRUE (isTimeInRange(1320, 360, 60));   // 01:00
  TEST_ASSERT_FALSE(isTimeInRange(1320, 360, 720));  // noon
  TEST_ASSERT_TRUE (isTimeInRange(1320, 360, 1320)); // start inclusive
  TEST_ASSERT_FALSE(isTimeInRange(1320, 360, 360));  // end exclusive
}

void test_isTimeInRange_equal_endpoints(void) {
  TEST_ASSERT_FALSE(isTimeInRange(120, 120, 120));
  TEST_ASSERT_FALSE(isTimeInRange(120, 120, 60));
}

// isDayInRange ----------------------------------------------------------------

void test_isDayInRange_normal(void) {
  // Mon–Fri = 1..5 (this codebase: 0=Sun..6=Sat)
  TEST_ASSERT_TRUE (isDayInRange(1, 5, 3));
  TEST_ASSERT_TRUE (isDayInRange(1, 5, 1));
  TEST_ASSERT_TRUE (isDayInRange(1, 5, 5));
  TEST_ASSERT_FALSE(isDayInRange(1, 5, 6));
  TEST_ASSERT_FALSE(isDayInRange(1, 5, 0));
}

void test_isDayInRange_wrap(void) {
  // Fri–Mon = 5..1 (wraps through Sat, Sun)
  TEST_ASSERT_TRUE (isDayInRange(5, 1, 5));   // Fri inclusive
  TEST_ASSERT_TRUE (isDayInRange(5, 1, 6));   // Sat
  TEST_ASSERT_TRUE (isDayInRange(5, 1, 0));   // Sun
  TEST_ASSERT_TRUE (isDayInRange(5, 1, 1));   // Mon inclusive
  TEST_ASSERT_FALSE(isDayInRange(5, 1, 3));   // Wed
}

// dateComp --------------------------------------------------------------------

void test_dateComp_count_up(void) {
  // Days since target earlier in year
  TEST_ASSERT_EQUAL_INT(31, dateComp(2024, 2, 1, 1, 1, true)); // Jan 1 -> Feb 1
}

void test_dateComp_count_down(void) {
  TEST_ASSERT_EQUAL_INT(31, dateComp(2024, 1, 1, 2, 1, false)); // Jan 1 -> Feb 1
}

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_daysInMonth_regular);
  RUN_TEST(test_daysInMonth_february_leap);
  RUN_TEST(test_daysInMonth_february_nonleap);
  RUN_TEST(test_daysInYear);
  RUN_TEST(test_dayOfWeek_known_dates);
  RUN_TEST(test_dayOfWeek_january_february_edge);
  RUN_TEST(test_nthSunday_positive);
  RUN_TEST(test_nthSunday_negative);
  RUN_TEST(test_nthSunday_us_dst_rules);
  RUN_TEST(test_dateToDayCount_year_start);
  RUN_TEST(test_dateToDayCount_progression);
  RUN_TEST(test_isTimeInRange_normal);
  RUN_TEST(test_isTimeInRange_midnight_cross);
  RUN_TEST(test_isTimeInRange_equal_endpoints);
  RUN_TEST(test_isDayInRange_normal);
  RUN_TEST(test_isDayInRange_wrap);
  RUN_TEST(test_dateComp_count_up);
  RUN_TEST(test_dateComp_count_down);
  return UNITY_END();
}
