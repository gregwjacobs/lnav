/**
 * Copyright (c) 2007-2012, Timothy Stack
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Timothy Stack nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file log_data_table.hh
 *
 * XXX This file has become a dumping ground for code and needs to be broken up
 * a bit.
 */

#ifndef _log_data_table_hh
#define _log_data_table_hh

#include <string>
#include <vector>

#include "lnav.hh"
#include "logfile.hh"
#include "data_parser.hh"
#include "column_namer.hh"
#include "log_vtab_impl.hh"

class log_data_table : public log_vtab_impl {
public:

    log_data_table(content_line_t template_line,
                   std::string table_name = "logline")
        : log_vtab_impl(table_name),
          ldt_template_line(template_line),
          ldt_parent_column_count(0) {
        logfile *lf = lnav_data.ld_log_source.find(template_line);
        log_format *format = lf->get_format();

        this->ldt_format_impl = lnav_data.ld_vtab_manager->lookup_impl(format->get_name());
    };

    void get_columns(std::vector<vtab_column> &cols)
    {
        content_line_t cl_copy = this->ldt_template_line;
        logfile *      lf      = lnav_data.ld_log_source.find(
            cl_copy);
        std::string val;
        struct line_range          body;
        string_attrs_t             sa;
        std::vector<logline_value> line_values;
        log_format *format = lf->get_format();

        if (this->ldt_format_impl != NULL) {
            this->ldt_format_impl->get_columns(cols);
        }
        this->ldt_parent_column_count = cols.size();
        lf->read_full_message(lf->begin() + cl_copy, val);
        format->annotate(val, sa, line_values);
        body = find_string_attr_range(sa, "body");
        if (body.lr_end == -1 || body.length() == 0) {
            this->ldt_schema_id.clear();
            return;
        }

        data_scanner ds(val, body.lr_start, body.lr_end);
        data_parser  dp(&ds);
        column_namer cn;

        dp.parse();

        for (data_parser::element_list_t::iterator pair_iter =
                 dp.dp_pairs.begin();
             pair_iter != dp.dp_pairs.end();
             ++pair_iter) {
            std::string key_str = dp.get_element_string(
                pair_iter->e_sub_elements->front());
            std::string colname  = cn.add_column(key_str);
            int         sql_type = SQLITE3_TEXT;
            const char *collator = NULL;
            char *      name;

            /* XXX LEAK */
            name = strdup(colname.c_str());
            switch (pair_iter->e_sub_elements->back().e_token) {
            case DT_IPV4_ADDRESS:
            case DT_IPV6_ADDRESS:
                collator = "ipaddress";
                break;

            case DT_NUMBER:
                sql_type = SQLITE_FLOAT;
                break;

            default:
                collator = "naturalnocase";
                break;
            }
            cols.push_back(vtab_column(name, sql_type, collator));
        }
        this->ldt_schema_id = dp.dp_schema_id;
    };

    bool next(log_cursor &lc, logfile_sub_source &lss)
    {
        lc.lc_curr_line = lc.lc_curr_line + vis_line_t(1);
        lc.lc_sub_index = 0;

        if (lc.lc_curr_line == (int)lss.text_line_count()) {
            return true;
        }

        content_line_t cl;

        cl = lss.at(lc.lc_curr_line);
        logfile *         lf      = lss.find(cl);
        logfile::iterator lf_iter = lf->begin() + cl;

        if (lf_iter->is_continued()) {
            return false;
        }

        if (lf_iter->has_schema() &&
            !lf_iter->match_schema(this->ldt_schema_id)) {
            return false;
        }

        string_attrs_t             sa;
        struct line_range          body;
        std::vector<logline_value> line_values;

        lf->read_full_message(lf_iter, this->ldt_current_line);
        lf->get_format()->annotate(this->ldt_current_line, sa, line_values);
        body = find_string_attr_range(sa, "body");
        if (body.lr_end == -1 || body.length() == 0) {
            return false;
        }

        data_scanner ds(this->ldt_current_line, body.lr_start, body.lr_end);
        data_parser  dp(&ds);
        dp.parse();

        lf_iter->set_schema(dp.dp_schema_id);

        /* The cached schema ID in the log line is not complete, so we still */
        /* need to check for a full match. */
        if (dp.dp_schema_id != this->ldt_schema_id) {
            return false;
        }

        this->ldt_pairs.clear();
        this->ldt_pairs.swap(dp.dp_pairs);

        return true;
    };

    void extract(logfile *lf,
                 const std::string &line,
                 std::vector<logline_value> &values)
    {
        int next_column = this->ldt_parent_column_count;

        this->ldt_format_impl->extract(lf, line, values);
        for (data_parser::element_list_t::iterator pair_iter =
                 this->ldt_pairs.begin();
             pair_iter != this->ldt_pairs.end();
             ++pair_iter) {
            const data_parser::element &pvalue = pair_iter->get_pair_value();
            const std::string           tmp    = this->ldt_current_line.substr(
                pvalue.e_capture.c_begin, pvalue.e_capture.length());

            switch (pvalue.value_token()) {
            case DT_NUMBER: {
                double d = 0;

                sscanf(tmp.c_str(), "%lf", &d);
                values.push_back(logline_value("", d));
            }
            break;

            default:
                values.push_back(logline_value("", tmp));
                break;
            }
            values.back().lv_column = next_column++;
        }
    };

private:
    const content_line_t     ldt_template_line;
    data_parser::schema_id_t ldt_schema_id;
    std::string ldt_current_line;
    data_parser::element_list_t ldt_pairs;
    log_vtab_impl *ldt_format_impl;
    int ldt_parent_column_count;
};
#endif
