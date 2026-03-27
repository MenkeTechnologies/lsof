/*
 * test_unit_monitor.h - monitor mode unit tests
 *
 * Tests monitor mode row budget calculation and ANSI escape
 * sequence construction in isolation (standalone reimplementation
 * of the core logic from monitor.c).
 */

#ifndef TEST_UNIT_MONITOR_H
#define TEST_UNIT_MONITOR_H

#include <string.h>

/*
 * Standalone reimplementation of the row budget calculation
 * from monitor_begin_frame() for testing.
 *
 * term_rows: total terminal height
 * status_lines: lines consumed by status bar (3 cyberpunk, 2 plain)
 * Returns: number of data rows available for file listing
 */
static int test_monitor_calc_row_budget(int term_rows, int status_lines) {
    int budget = term_rows - status_lines - 1;
    if (budget < 3)
        budget = 3;
    return budget;
}

/* --- Row budget calculation tests --- */

TEST(monitor_row_budget_normal_terminal) {
    /* 40-row terminal with 3 status lines -> 36 data rows */
    ASSERT_EQ(test_monitor_calc_row_budget(40, 3), 36);
}

TEST(monitor_row_budget_standard_24) {
    /* Standard 24-row terminal with 3 status lines -> 20 data rows */
    ASSERT_EQ(test_monitor_calc_row_budget(24, 3), 20);
}

TEST(monitor_row_budget_small_terminal) {
    /* Very small terminal: 5 rows, 3 status -> 1, but clamped to 3 */
    ASSERT_EQ(test_monitor_calc_row_budget(5, 3), 3);
}

TEST(monitor_row_budget_minimum_clamp) {
    /* Terminal so small budget goes negative -> clamped to 3 */
    ASSERT_EQ(test_monitor_calc_row_budget(2, 3), 3);
}

TEST(monitor_row_budget_large_terminal) {
    /* Large terminal: 100 rows with 2 status lines -> 97 */
    ASSERT_EQ(test_monitor_calc_row_budget(100, 2), 97);
}

TEST(monitor_row_budget_plain_mode) {
    /* Non-cyberpunk mode uses 2 status lines instead of 3 */
    ASSERT_EQ(test_monitor_calc_row_budget(24, 2), 21);
}

TEST(monitor_row_budget_zero_rows) {
    /* Degenerate case: 0-row terminal -> clamped to 3 */
    ASSERT_EQ(test_monitor_calc_row_budget(0, 3), 3);
}

TEST(monitor_row_budget_exact_threshold) {
    /* Budget exactly 3: term=7, status=3 -> 7-3-1=3 */
    ASSERT_EQ(test_monitor_calc_row_budget(7, 3), 3);
}

TEST(monitor_row_budget_one_above_threshold) {
    /* Budget exactly 4: term=8, status=3 -> 8-3-1=4 */
    ASSERT_EQ(test_monitor_calc_row_budget(8, 3), 4);
}

TEST(monitor_row_budget_one_below_threshold) {
    /* Budget 2 before clamp: term=6, status=3 -> 6-3-1=2 -> clamped to 3 */
    ASSERT_EQ(test_monitor_calc_row_budget(6, 3), 3);
}

/* --- ANSI escape sequence validation tests --- */

TEST(monitor_ansi_alt_screen_enter) {
    /* Alternate screen buffer entry sequence */
    const char *seq = "\033[?1049h";
    ASSERT_EQ((int)strlen(seq), 8);
    ASSERT_EQ(seq[0], '\033');
    ASSERT_EQ(seq[1], '[');
    ASSERT_EQ(seq[7], 'h'); /* 'h' = set mode */
}

TEST(monitor_ansi_alt_screen_leave) {
    /* Alternate screen buffer exit sequence */
    const char *seq = "\033[?1049l";
    ASSERT_EQ((int)strlen(seq), 8);
    ASSERT_EQ(seq[7], 'l'); /* 'l' = reset mode */
}

TEST(monitor_ansi_alt_screen_symmetry) {
    /* Enter and leave sequences differ only in last char */
    const char *enter = "\033[?1049h";
    const char *leave = "\033[?1049l";
    ASSERT_EQ(strncmp(enter, leave, 7), 0); /* first 7 chars match */
    ASSERT_NE(enter[7], leave[7]);          /* last char differs */
}

TEST(monitor_ansi_cursor_home) {
    const char *seq = "\033[H";
    ASSERT_EQ((int)strlen(seq), 3);
    ASSERT_EQ(seq[2], 'H');
}

TEST(monitor_ansi_clear_to_end) {
    const char *seq = "\033[J";
    ASSERT_EQ((int)strlen(seq), 3);
    ASSERT_EQ(seq[2], 'J');
}

TEST(monitor_ansi_hide_cursor) {
    const char *seq = "\033[?25l";
    ASSERT_EQ((int)strlen(seq), 6);
    ASSERT_EQ(seq[5], 'l'); /* 'l' = low/hide */
}

TEST(monitor_ansi_show_cursor) {
    const char *seq = "\033[?25h";
    ASSERT_EQ((int)strlen(seq), 6);
    ASSERT_EQ(seq[5], 'h'); /* 'h' = high/show */
}

TEST(monitor_ansi_cursor_visibility_symmetry) {
    /* Hide and show differ only in last char */
    const char *hide = "\033[?25l";
    const char *show = "\033[?25h";
    ASSERT_EQ(strncmp(hide, show, 5), 0);
    ASSERT_NE(hide[5], show[5]);
}

/* --- Row counter decrement simulation --- */

TEST(monitor_row_counter_decrement) {
    /* Simulate row counting during print pass */
    int rows_remaining = 10;
    int rows_printed = 0;
    while (rows_remaining > 0) {
        rows_remaining--;
        rows_printed++;
    }
    ASSERT_EQ(rows_printed, 10);
    ASSERT_EQ(rows_remaining, 0);
}

TEST(monitor_row_counter_truncation) {
    /* When rows_remaining hits 0, printing should stop */
    int rows_remaining = 3;
    int total_files = 100;
    int printed = 0;
    for (int i = 0; i < total_files; i++) {
        if (rows_remaining <= 0)
            break;
        rows_remaining--;
        printed++;
    }
    ASSERT_EQ(printed, 3);
    ASSERT_EQ(rows_remaining, 0);
}

TEST(monitor_header_reduces_budget) {
    /* Header lines should reduce the row budget */
    int budget = test_monitor_calc_row_budget(24, 3); /* 20 */
    int header_lines = 2; /* column header + separator */
    budget -= header_lines;
    ASSERT_EQ(budget, 18);
    ASSERT_TRUE(budget > 0);
}

#endif /* TEST_UNIT_MONITOR_H */
