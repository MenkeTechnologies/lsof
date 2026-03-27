/*
 * test_unit.c - unit tests for lsof internal functions
 *
 * Tests pure/isolated functions that can be compiled and tested without
 * the full lsof dialect-specific build infrastructure.
 *
 * Each section is in its own header file for organization.
 */

#include "test.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#include "test_unit_fields.h"
#include "test_unit_x2dev.h"
#include "test_unit_hash.h"
#include "test_unit_strings.h"
#include "test_unit_format.h"
#include "test_unit_compare.h"
#include "test_unit_memleak.h"
#include "test_unit_network.h"
#include "test_unit_fd.h"
#include "test_unit_path.h"
#include "test_unit_devhash.h"
#include "test_unit_portcache.h"
#include "test_unit_uid.h"
#include "test_unit_stat.h"
#include "test_unit_readlink.h"
#include "test_unit_strftime.h"
#include "test_unit_devlookup.h"
#include "test_unit_regex.h"
#include "test_unit_environ.h"
#include "test_unit_json.h"
#include "test_unit_leak.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    TEST_SUITE("LSOF UNIT TESTS");

    /* --- fields & flags --- */
    RUN(field_ids_are_unique);
    RUN(field_indices_sequential);
    RUN(field_id_values);
    RUN(field_names_non_null);
    RUN(field_names_non_empty);
    RUN(field_ids_are_printable_ascii);
    RUN(field_total_count);
    RUN(crossover_flags);
    RUN(fsv_flags_no_overlap);
    RUN(fsv_flags_combine);

    /* --- x2dev --- */
    RUN(x2dev_simple_hex);
    RUN(x2dev_with_0x_prefix);
    RUN(x2dev_with_0X_prefix);
    RUN(x2dev_uppercase);
    RUN(x2dev_stops_at_space);
    RUN(x2dev_stops_at_comma);
    RUN(x2dev_empty_string);
    RUN(x2dev_invalid_char);
    RUN(x2dev_just_0x);
    RUN(x2dev_zero);
    RUN(x2dev_single_digit);
    RUN(x2dev_mixed_case);
    RUN(x2dev_max_byte);
    RUN(x2dev_leading_zeros);
    RUN(x2dev_stops_at_null_terminator);
    RUN(x2dev_two_digits);
    RUN(x2dev_0x_with_zero);
    RUN(x2dev_all_f_prefix);
    RUN(x2dev_stops_at_invalid_after_valid);
    RUN(x2dev_consecutive_zeros);

    /* --- hash --- */
    RUN(hashport_range);
    RUN(hashport_distribution);
    RUN(hashport_deterministic);
    RUN(hashport_adjacent_ports_differ);
    RUN(hashport_max_port);
    RUN(hashport_zero);
    RUN(hashport_common_ports_in_range);
    RUN(hashbyname_range);
    RUN(hashbyname_deterministic);
    RUN(hashbyname_different_strings_differ);
    RUN(hashbyname_empty_string);
    RUN(hashbyname_case_sensitive);
    RUN(hashbyname_power_of_two_mod);
    RUN(hashbyname_distribution);
    RUN(hashbyname_no_trivial_collision_set);
    RUN(hashbyname_long_string);
    RUN(hashbyname_single_char_spread);

    /* --- strings --- */
    RUN(safestrlen_normal_string);
    RUN(safestrlen_empty);
    RUN(safestrlen_null);
    RUN(safestrlen_with_tab);
    RUN(safestrlen_with_newline);
    RUN(safestrlen_space_not_printable);
    RUN(safestrlen_space_printable);
    RUN(safestrlen_0xff_char);
    RUN(safestrlen_high_byte);
    RUN(safestrlen_all_printable);
    RUN(safestrlen_multiple_control);
    RUN(safestrlen_mixed_content);
    RUN(safestrlen_backspace);
    RUN(safestrlen_formfeed);
    RUN(safestrlen_cr);
    RUN(safestrlen_single_printable);
    RUN(safestrlen_space_default);
    RUN(safestrlen_space_as_nonprint);
    RUN(safepup_tab);
    RUN(safepup_newline);
    RUN(safepup_backspace);
    RUN(safepup_formfeed);
    RUN(safepup_cr);
    RUN(safepup_ctrl_a);
    RUN(safepup_null_char);
    RUN(safepup_0xff);
    RUN(safepup_high_byte);
    RUN(safepup_null_cl);
    RUN(safepup_printable_range);
    RUN(safepup_all_control_chars_escaped);
    RUN(safepup_del_char);
    RUN(safestrprt_printable_to_buffer);
    RUN(safestrprt_control_char_format);
    RUN(mkstrcpy_basic);
    RUN(mkstrcpy_empty);
    RUN(mkstrcpy_null_source);
    RUN(mkstrcpy_null_lenptr);
    RUN(mkstrcpy_long_string);
    RUN(mkstrcpy_is_independent_copy);
    RUN(mkstrcat_two_strings);
    RUN(mkstrcat_three_strings);
    RUN(mkstrcat_with_length_limits);
    RUN(mkstrcat_empty_strings);
    RUN(mkstrcat_null_strings);
    RUN(mkstrcat_mixed_null);
    RUN(mkstrcat_zero_length);
    RUN(mkstrcat_path_building);
    RUN(mkstrcat_all_length_limited);
    RUN(mkstrcat_only_third);
    RUN(mkstrcat_long_concat);

    /* --- format --- */
    RUN(snpf_basic);
    RUN(snpf_truncation);
    RUN(snpf_zero_buffer);
    RUN(snpf_integer);
    RUN(snpf_hex);
    RUN(snpf_multiple_args);
    RUN(snpf_empty_format);
    RUN(snpf_percent_literal);
    RUN(snpf_long_integer);
    RUN(snpf_negative_integer);
    RUN(snpf_unsigned);
    RUN(snpf_octal);
    RUN(snpf_char);
    RUN(snpf_width_right);
    RUN(snpf_width_left);
    RUN(snpf_zero_pad);
    RUN(snpf_string_precision);
    RUN(snpf_combined_format);
    RUN(snpf_exact_fit);
    RUN(snpf_one_byte_buffer);
    RUN(fmtnum_decimal_positive);
    RUN(fmtnum_decimal_negative);
    RUN(fmtnum_decimal_zero);
    RUN(fmtnum_hex);
    RUN(fmtnum_octal);
    RUN(fmtnum_width_right_justify);
    RUN(fmtnum_width_left_justify);
    RUN(fmtnum_width_zero_pad);
    RUN(fmtnum_neg_with_zero_pad);
    RUN(fmtnum_hex_large);
    RUN(fmtstr_basic);
    RUN(fmtstr_null_becomes_label);
    RUN(fmtstr_right_justify);
    RUN(fmtstr_left_justify);
    RUN(fmtstr_maxwidth_truncate);
    RUN(fmtstr_empty);
    RUN(fmtstr_width_with_truncate);
    RUN(kptr_nonzero);
    RUN(kptr_zero);
    RUN(kptr_null_buf_uses_static);
    RUN(kptr_small_buf_truncates);

    /* --- compare --- */
    RUN(compdev_equal);
    RUN(compdev_rdev_less);
    RUN(compdev_rdev_greater);
    RUN(compdev_inode_less);
    RUN(compdev_inode_greater);
    RUN(compdev_name_compare);
    RUN(compdev_sort_array);
    RUN(compdev_null_names);
    RUN(compdev_stability);
    RUN(compdev_sort_by_rdev_first);
    RUN(compdev_sort_by_inode_second);
    RUN(compdev_sort_by_name_third);
    RUN(compdev_large_rdev);
    RUN(compdev_large_inode);
    RUN(compdev_empty_names);
    RUN(compdev_one_null_name);
    RUN(comppid_equal);
    RUN(comppid_less);
    RUN(comppid_greater);
    RUN(comppid_sort);
    RUN(comppid_negative_pids);
    RUN(comppid_duplicate_pids);
    RUN(pid_max_int);
    RUN(pid_zero_vs_negative);
    RUN(cmp_int_lst_equal);
    RUN(cmp_int_lst_less);
    RUN(cmp_int_lst_greater);
    RUN(cmp_int_lst_negative);
    RUN(cmp_int_lst_sort);
    RUN(cmp_int_lst_zero);
    RUN(cmp_int_lst_int_extremes);
    RUN(find_int_lst_found);
    RUN(find_int_lst_not_found);
    RUN(find_int_lst_first);
    RUN(find_int_lst_last);
    RUN(find_int_lst_single_hit);
    RUN(find_int_lst_single_miss);
    RUN(find_int_lst_large_sorted);
    RUN(devnum_major_minor);
    RUN(devnum_makedev);
    RUN(devnum_zero);
    RUN(rmdupdev_no_dups);
    RUN(rmdupdev_all_same);
    RUN(rmdupdev_adjacent_dups);
    RUN(rmdupdev_single);
    RUN(rmdupdev_empty);
    RUN(rmdupdev_same_rdev_diff_inode);
    RUN(rmdupdev_interleaved);
    RUN(rmdupdev_two_elements_same);
    RUN(rmdupdev_two_elements_different);

    /* --- memleak --- */
    RUN(memleak_enter_network_address_hn_freed);
    RUN(memleak_enter_network_address_error_frees_all);
    RUN(memleak_enter_fd_lst_dup_frees_nm);
    RUN(memleak_enter_fd_lst_dup_numeric_no_leak);
    RUN(memleak_enter_efsys_readlink_fail_frees_ec);
    RUN(memleak_enter_efsys_dup_frees_ec);
    RUN(memleak_enter_efsys_success_frees_ec);
    RUN(memleak_enter_efsys_rdlnk_no_double_free);
    RUN(memleak_mkstrcpy_basic);
    RUN(memleak_mkstrcpy_null);
    RUN(memleak_mkstrcat_basic);
    RUN(memleak_enter_nm_frees_old);
    RUN(memleak_free_lproc_pattern);
    RUN(memleak_safe_realloc_success);
    RUN(memleak_unsafe_realloc_pattern);
    RUN(memleak_add_nma_realloc_pattern);
    RUN(memleak_alloc_lproc_realloc_pattern);
    RUN(memleak_hostcache_realloc_pattern);
    RUN(memleak_sn_realloc_pattern);
    RUN(memleak_cmdrx_realloc_pattern);
    RUN(memleak_enter_id_realloc_pattern);
    RUN(memleak_suid_realloc_pattern);
    RUN(memleak_sort_ptr_realloc_pattern);
    RUN(memleak_dstk_realloc_pattern);
    RUN(memleak_ipstate_realloc_pattern);
    RUN(memleak_str_list_cleanup);
    RUN(memleak_double_buffer_pattern);
    RUN(memleak_nested_alloc_pattern);
    RUN(memleak_nwad_exit_frees_arg);
    RUN(memleak_nwad_exit_arg_null_safe);
    RUN(memleak_readlink_path_too_long_frees_stack);
    RUN(memleak_readlink_path_too_long_empty_stack);
    RUN(memleak_darwin_safe_realloc_success);
    RUN(memleak_darwin_safe_realloc_failure);
    RUN(memleak_darwin_adev_realloc_pattern);

    /* --- network --- */
    RUN(ipv4_basic);
    RUN(ipv4_loopback);
    RUN(ipv4_all_zeros);
    RUN(ipv4_broadcast);
    RUN(ipv4_with_port);
    RUN(ipv4_stops_at_colon);
    RUN(ipv4_too_few_octets);
    RUN(ipv4_too_many_octets);
    RUN(ipv4_octet_too_large);
    RUN(ipv4_empty);
    RUN(ipv4_alpha);
    RUN(ipv4_leading_alpha);
    RUN(ipv4_null_addr);
    RUN(ipv4_short_buffer);
    RUN(ipv4_trailing_dot);
    RUN(ipv4_class_a);
    RUN(ipv4_class_c);
    RUN(ipv4_single_digit_octets);
    RUN(ipv4_octet_255_boundary);
    RUN(ipv4_second_octet_too_large);
    RUN(ipv4_last_octet_too_large);
    RUN(ipv4_double_dot);
    RUN(ipv4_port_extraction);
    RUN(ipv6_detect_full);
    RUN(ipv6_detect_loopback);
    RUN(ipv6_detect_ipv4);
    RUN(ipv6_detect_single_colon);
    RUN(ipv6_detect_full_expanded);
    RUN(ipv6_detect_link_local);
    RUN(ipv6_detect_all_zeros);
    RUN(ipv6_detect_empty);
    RUN(ipv6_detect_no_colon);
    RUN(af_name_unspec);
    RUN(af_name_unix);
    RUN(af_name_inet);
    RUN(af_name_inet6);
    RUN(af_name_unknown);
    RUN(socktype_stream);
    RUN(socktype_dgram);
    RUN(socktype_raw);
    RUN(socktype_unknown);

    /* --- fd --- */
    RUN(ckfd_range_basic);
    RUN(ckfd_range_single_digits);
    RUN(ckfd_range_large);
    RUN(ckfd_range_equal_rejected);
    RUN(ckfd_range_reversed_rejected);
    RUN(ckfd_range_non_digit_rejected);
    RUN(ckfd_range_non_digit_hi_rejected);
    RUN(ckfd_range_bad_pointers);
    RUN(parse_fd_zero);
    RUN(parse_fd_normal);
    RUN(parse_fd_max);
    RUN(parse_fd_overflow);
    RUN(parse_fd_negative);
    RUN(parse_fd_alpha);
    RUN(parse_fd_mixed);
    RUN(parse_fd_empty);
    RUN(ck_fd_status_empty_list);
    RUN(ck_fd_status_name_included);
    RUN(ck_fd_status_name_excluded);
    RUN(ck_fd_status_num_in_range);
    RUN(ck_fd_status_num_out_of_range);
    RUN(ck_fd_status_name_not_found);
    RUN(ck_fd_status_leading_spaces);
    RUN(ck_fd_status_chain);
    RUN(str_lst_include);
    RUN(str_lst_exclude);
    RUN(str_lst_multiple);
    RUN(str_lst_empty_rejected);
    RUN(str_lst_null_rejected);
    RUN(str_lst_caret_only);

    /* --- path --- */
    RUN(path_absolute_yes);
    RUN(path_absolute_no);
    RUN(path_absolute_empty);
    RUN(path_absolute_null);
    RUN(path_depth_root);
    RUN(path_depth_deep);
    RUN(path_depth_empty);
    RUN(path_absolute_just_slash);
    RUN(path_absolute_double_slash);
    RUN(path_absolute_dot);
    RUN(path_absolute_dotdot);
    RUN(path_depth_trailing_slash);
    RUN(path_depth_double_slash);
    RUN(path_depth_null);
    RUN(path_depth_no_slash);

    /* --- devhash --- */
    RUN(sfhash_range);
    RUN(sfhash_deterministic);
    RUN(sfhash_different_inodes_differ);
    RUN(sfhash_different_majors_differ);
    RUN(sfhash_different_minors_differ);
    RUN(sfhash_power_of_two_mod);
    RUN(sfhash_distribution);
    RUN(sfhash_zero_inputs);
    RUN(ncache_hash_range);
    RUN(ncache_hash_deterministic);
    RUN(ncache_hash_different_inodes_differ);
    RUN(ncache_hash_different_addrs_differ);
    RUN(ncache_hash_distribution);

    /* --- portcache --- */
    RUN(port_table_build);
    RUN(port_table_lookup_hit);
    RUN(port_table_lookup_miss);
    RUN(port_table_collision_chain);
    RUN(port_table_overwrite);
    RUN(port_table_all_common_ports);
    RUN(port_table_max_port);
    RUN(port_table_zero_port);

    /* --- uid --- */
    RUN(uid_lookup_root);
    RUN(uid_lookup_current_user);
    RUN(uid_lookup_nonexistent);
    RUN(uid_cache_hit);
    RUN(uid_hash_range);
    RUN(uid_hash_deterministic);
    RUN(uid_hash_distribution);
    RUN(uid_cache_multiple_users);
    RUN(uid_cache_collision_chain);

    /* --- stat --- */
    RUN(stat_regular_file);
    RUN(stat_directory);
    RUN(stat_dev_null);
    RUN(stat_nonexistent);
    RUN(stat_null_path);
    RUN(stat_null_buf);
    RUN(stat_file_size);
    RUN(stat_empty_file);
    RUN(stat_permissions);
    RUN(stat_inode_nonzero);
    RUN(stat_nlink);
    RUN(stat_dev_zero);
    RUN(lstat_symlink);
    RUN(lstat_follows_regular);
    RUN(stat_resolves_symlink);
    RUN(lstat_null_path);
    RUN(lstat_null_buf);
    RUN(file_type_name_reg);
    RUN(file_type_name_dir);
    RUN(file_type_name_chr);
    RUN(file_type_name_blk);
    RUN(file_type_name_fifo);
    RUN(file_type_name_lnk);
    RUN(file_type_name_sock);
    RUN(file_type_name_unknown);
    RUN(stat_dev_major_minor);
    RUN(stat_pipe_via_fifo);

    /* --- readlink --- */
    RUN(readlink_valid_symlink);
    RUN(readlink_not_a_symlink);
    RUN(readlink_nonexistent);
    RUN(readlink_null_path);
    RUN(readlink_chain);
    RUN(readlink_dev_stdin);
    RUN(readlink_relative_target);
    RUN(readlink_self_link);

    /* --- strftime --- */
    RUN(strftime_basic);
    RUN(strftime_full_date);
    RUN(strftime_time_only);
    RUN(strftime_null_buf);
    RUN(strftime_zero_bufsz);
    RUN(strftime_null_fmt);
    RUN(strftime_small_buffer);
    RUN(strftime_epoch);
    RUN(strftime_day_of_week);
    RUN(strftime_percent_literal);
    RUN(strftime_mixed_text);
    RUN(strftime_current_time_reasonable);

    /* --- devlookup --- */
    RUN(devlookup_found);
    RUN(devlookup_not_found);
    RUN(devlookup_first_element);
    RUN(devlookup_last_element);
    RUN(devlookup_single_element_found);
    RUN(devlookup_single_element_miss);
    RUN(devlookup_same_rdev_diff_inode);
    RUN(devlookup_same_rdev_miss_inode);
    RUN(devlookup_empty_table);
    RUN(devlookup_large_values);
    RUN(devlookup_zero_device);
    RUN(devlookup_sort_then_search);
    RUN(devlookup_sort_preserves_names);
    RUN(devlookup_binary_search_boundary);

    /* --- regex/matching --- */
    RUN(cmd_match_exact);
    RUN(cmd_match_prefix);
    RUN(cmd_match_no_match);
    RUN(cmd_match_empty_pattern);
    RUN(cmd_match_empty_cmd);
    RUN(cmd_match_both_empty);
    RUN(cmd_match_null_pattern);
    RUN(cmd_match_null_cmd);
    RUN(cmd_match_longer_pattern);
    RUN(cmd_match_case_sensitive);
    RUN(cmd_match_with_numbers);
    RUN(cmd_match_with_special_chars);
    RUN(glob_match_exact);
    RUN(glob_match_star_all);
    RUN(glob_match_star_prefix);
    RUN(glob_match_star_suffix);
    RUN(glob_match_star_middle);
    RUN(glob_match_question);
    RUN(glob_match_no_match);
    RUN(glob_match_star_empty);
    RUN(glob_match_multiple_stars);
    RUN(glob_match_question_no_match);
    RUN(glob_match_empty_pattern);
    RUN(glob_match_star_only);
    RUN(glob_match_double_star);
    RUN(glob_match_complex);
    RUN(glob_match_complex_no_match);
    RUN(charclass_single);
    RUN(charclass_range);
    RUN(charclass_range_boundary_low);
    RUN(charclass_range_boundary_high);
    RUN(charclass_range_miss);
    RUN(charclass_digit_range);
    RUN(charclass_digit_range_miss);
    RUN(charclass_multiple_ranges);

    /* --- environ --- */
    RUN(env_path_exists);
    RUN(env_home_exists);
    RUN(env_nonexistent);
    RUN(env_null_name);
    RUN(env_user_exists);
    RUN(pid_valid_positive);
    RUN(pid_valid_zero);
    RUN(pid_valid_negative);
    RUN(pid_valid_current);
    RUN(process_exists_self);
    RUN(process_exists_parent);
    RUN(process_exists_init);
    RUN(process_exists_invalid);
    RUN(pid_list_single);
    RUN(pid_list_multiple);
    RUN(pid_list_spaces);
    RUN(pid_list_empty);
    RUN(pid_list_null);
    RUN(pid_list_single_comma);
    RUN(pid_list_trailing_comma);
    RUN(pid_list_large);

    /* --- json --- */
    RUN(json_escape_plain_string);
    RUN(json_escape_empty_string);
    RUN(json_escape_null_becomes_null);
    RUN(json_escape_double_quote);
    RUN(json_escape_backslash);
    RUN(json_escape_newline);
    RUN(json_escape_tab);
    RUN(json_escape_carriage_return);
    RUN(json_escape_backspace);
    RUN(json_escape_formfeed);
    RUN(json_escape_control_char_unicode);
    RUN(json_escape_mixed_specials);
    RUN(json_escape_path_with_spaces);
    RUN(json_escape_printable_ascii_passthrough);
    RUN(json_escape_all_special_chars);
    RUN(json_array_brackets);
    RUN(json_object_structure);
    RUN(json_file_fields);

    /* --- leak detection --- */
    RUN(leak_hash_range);
    RUN(leak_hash_deterministic);
    RUN(leak_hash_distribution);
    RUN(leak_hash_adjacent_pids_differ);
    RUN(leak_find_or_create_new);
    RUN(leak_find_or_create_existing);
    RUN(leak_find_or_create_pid_reuse_resets);
    RUN(leak_find_or_create_different_pids);
    RUN(leak_record_single_sample);
    RUN(leak_record_increasing_triggers_flag);
    RUN(leak_record_decrease_resets_count);
    RUN(leak_record_equal_resets_count);
    RUN(leak_record_threshold_2);
    RUN(leak_record_just_below_threshold);
    RUN(leak_history_circular_buffer);
    RUN(leak_history_wraps_correctly);
    RUN(leak_multiple_processes_independent);
    RUN(leak_cleanup_frees_all);
    RUN(leak_flag_stays_set_after_more_records);

    TEST_REPORT();
}
