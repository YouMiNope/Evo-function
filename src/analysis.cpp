#include "analysis.hpp"

#include <math.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <iomanip>

BasicAnalyser::BasicAnalyser():
    dp_matrix(nullptr)
{

}

BasicAnalyser::~BasicAnalyser()
{
    free(dp_matrix);
}

void BasicAnalyser::comp(Sequence &a, Sequence &b, GeneLibrary::mutate_prob &prob)
{
    dp_matrix = (score *)realloc(dp_matrix, (a.len + 1) * (b.len + 1) * sizeof(score));

    m_seq_a = a;
    m_seq_b = b;
    my_prob = prob;
    compute_score(a, b);
}

void BasicAnalyser::compute_score(Sequence &a, Sequence &b)
{
    dp_matrix[0].score = 0.f;
    dp_matrix[0].flags = 0x00;
    for(size_t i = 0; i < a.len; i++)
    {
        dp_matrix[(i + 1) * b.len + 0].score = dp_matrix[i * b.len + 0].score + get_indel_score();
        dp_matrix[(i + 1) * b.len + 0].flags = SEQB_INSEL;
    }
    for(size_t j = 0; j < b.len; j++)
    {
        dp_matrix[0 * b.len + (j + 1)].score = dp_matrix[0 * b.len + j].score + get_indel_score();
        dp_matrix[0 * b.len + (j + 1)].flags = SEQA_INSEL;
    }
    for(size_t i = 0; i < a.len; i++)
    {
        for(size_t j = 0; j < b.len; j++)
        {
            float a_ins_score = dp_matrix[(i + 1) * b.len + j].score + get_indel_score();
            float b_ins_score = dp_matrix[i * b.len + (j + 1)].score + get_indel_score();
            float match_score = dp_matrix[i * b.len + j].score + get_match_score(a.p_context[i], b.p_context[j]);

            float _max = std::max(a_ins_score, b_ins_score);
            _max = std::max(_max, match_score);

            uint8_t _flag = 0x00;
            if(_max == a_ins_score)     _flag |= SEQA_INSEL;
            if(_max == b_ins_score)     _flag |= SEQB_INSEL;
            if(_max == match_score)     _flag |= NO_INSEL;
            dp_matrix[(i + 1) * b.len + (j + 1)].score = _max;
            dp_matrix[(i + 1) * b.len + (j + 1)].flags = _flag;
        }
    }
}

float BasicAnalyser::get_indel_score()
{
    return -(std::min(my_prob.add, my_prob.del));
}

float BasicAnalyser::get_match_score(uint8_t a, uint8_t b)
{
    uint8_t mis_count = 0;
    uint8_t flips = a ^ b;
    while(flips != 0)
    {
        if(flips & 1 == 1)  mis_count++;
        flips >>= 1;
    }
    return -(my_prob.flip * mis_count);
}

void BasicAnalyser::print()
{
    // _print_at(m_seq_a.len, m_seq_b.len);
    size_t i = m_seq_a.len - 1;
    size_t j = m_seq_b.len - 1;
    std::string a_line;
    std::string b_line;
    std::string d_line;
    std::stringstream a_stm;
    std::stringstream b_stm;
    std::stringstream d_stm;
    while(i > 0 && j > 0)
    {
        if(dp_matrix[(i + 1) * m_seq_b.len + (j + 1)].flags & NO_INSEL)
        {
            if(m_seq_a.p_context[i] == m_seq_b.p_context[j])
            {
                d_stm << " " << " :";
            }
            else
            {
                d_stm << " " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (m_seq_a.p_context[i] ^ m_seq_b.p_context[j]);
            }
            a_stm << " " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (int)m_seq_a.p_context[i--];
            b_stm << " " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (int)m_seq_b.p_context[j--];
        }
        else if(dp_matrix[i * m_seq_b.len + (j + 1)].flags & SEQA_INSEL)
        {
            a_stm << " " << "__";
            b_stm << " " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (int)m_seq_b.p_context[j--];
            d_stm << " " << "  ";
        }
        else if(dp_matrix[(i + 1) * m_seq_b.len + j].flags & SEQB_INSEL)
        {
            a_stm << " " << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (int)m_seq_a.p_context[i--];
            b_stm << " " << "__";
            d_stm << " " << "  ";
        }
    }
    a_line = a_stm.str();
    b_line = b_stm.str();
    d_line = d_stm.str();
    for(i = 0; i < (a_line.length() / (32 * 3)); i++)
    {
        std::cout << '\n';
        for(j = 0; j < 32 * 3; j++)
        {
            std::cout << a_line[i * 32 * 3 + j];
        }
        std::cout << '\n';
        for(j = 0; j < 32 * 3; j++)
        {
            std::cout << d_line[i * 32 * 3 + j];
        }
        std::cout << '\n';
        for(j = 0; j < 32 * 3; j++)
        {
            std::cout << b_line[i * 32 * 3 + j];
        }
        std::cout << '\n';
    }
}

void BasicAnalyser::_print_at(size_t i, size_t j)
{
    if(i > 0 && j > 0)
    {
        if(dp_matrix[i * m_seq_b.len + j].flags & NO_INSEL)
        {
            _print_at(i - 1, j - 1);
            printf("%02X %02X\n", m_seq_a.p_context[i], m_seq_b.p_context[j]);
        }
        else if(dp_matrix[i * m_seq_b.len + j].flags & SEQA_INSEL)
        {
            _print_at(i, j - 1);
            printf("__ %02X\n", m_seq_b.p_context[j]);
        }
        else if(dp_matrix[i * m_seq_b.len + j].flags & SEQB_INSEL)
        {
            _print_at(i - 1, j);
            printf("%02X __\n", m_seq_a.p_context[i]);
        }
    }
    return;
}
