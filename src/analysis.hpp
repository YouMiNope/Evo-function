#ifndef ANALYSIS_HPP
#define ANALYSIS_HPP

#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>



#define SEQA_INSEL 0x01
#define SEQB_INSEL 0x02
#define NO_INSEL   0x04


namespace GeneLibrary
{
    struct mutate_prob
    {
        uint8_t flip = 32;
        uint8_t add  = 32;
        uint8_t del  = 32;
    };
}

class Sequence
{
public:
    size_t len;
    uint8_t *p_context;

    Sequence():
        len(0),
        p_context(nullptr)
    {}
    ~Sequence()
    {
        // free(p_context);
    }
};

typedef struct
{
    uint8_t flags;
    float score;
}score;


class BasicAnalyser
{
public:
    BasicAnalyser();
    ~BasicAnalyser();

    void comp(Sequence &a, Sequence &b, GeneLibrary::mutate_prob &);
    void print();

protected:
    void compute_score(Sequence &a, Sequence &b);
    float get_match_score(uint8_t a, uint8_t b);
    float get_indel_score();

    void _print_at(size_t i, size_t j);

private:
    score *dp_matrix;
    GeneLibrary::mutate_prob my_prob;
    Sequence m_seq_a;
    Sequence m_seq_b;
};

#endif

