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
 * @file pcrepp.cc
 */

#include "config.h"

#include "pcrepp.hh"

const int JIT_STACK_MIN_SIZE = 32 * 1024;
const int JIT_STACK_MAX_SIZE = 512 * 1024;

pcre_context::capture_t *pcre_context::operator[](const char *name) const
{
    capture_t *retval = NULL;
    int index;

    index = this->pc_pcre->name_index(name);
    if (index != PCRE_ERROR_NOSUBSTRING) {
        retval = &this->pc_captures[index + 1];
    }

    return retval;
}

#ifdef PCRE_STUDY_JIT_COMPILE
pcre_jit_stack *pcrepp::jit_stack(void)
{
    static pcre_jit_stack *retval = NULL;

    if (retval == NULL) {
        retval = pcre_jit_stack_alloc(JIT_STACK_MIN_SIZE, JIT_STACK_MAX_SIZE);
    }

    return retval;
}

#else
#warning "pcrejit is not available, search performance will be degraded"

void pcrepp::pcre_free_study(pcre_extra *extra)
{
    free(extra);
}
#endif
