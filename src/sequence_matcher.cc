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
 */

#include "config.h"

#include <openssl/sha.h>

#include "sequence_matcher.hh"

using namespace std;

sequence_matcher::sequence_matcher(field_col_t &example)
{
    for (field_col_t::iterator col_iter = example.begin();
         col_iter != example.end();
         ++col_iter) {
        std::string first_value;
        field       sf;

        sf.sf_value = *col_iter;
        for (field_row_t::iterator row_iter = (*col_iter).begin();
             row_iter != (*col_iter).end();
             ++row_iter) {
            if (row_iter == (*col_iter).begin()) {
                first_value = *row_iter;
            }
            else if (first_value != *row_iter) {
                sf.sf_type = FT_CONSTANT;
            }
        }
        if (sf.sf_type == FT_VARIABLE) {
            sf.sf_value.clear();
        }
        this->sm_fields.push_back(sf);
    }
    this->sm_count = example.front().size();
}

void sequence_matcher::identity(const std::vector<string> &values,
                                id_t &id_out)
{
    SHA_CTX context;
    int     lpc = 0;

    SHA_Init(&context);
    for (std::list<field>::iterator iter = sm_fields.begin();
         iter != sm_fields.end();
         ++iter, lpc++) {
        if (iter->sf_type == FT_VARIABLE) {
            SHA_Update(&context,
                       values[lpc].c_str(),
                       values[lpc].length() + 1);
        }
    }
    SHA_Final(id_out.ba_data, &context);
}
