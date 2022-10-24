#define MATRIX_SIZE 16
#define MATRIX_LINE 4
#define INPUT_MAT_COUNT 2

#define POUT_MAT(i, j) (MATRIX_SIZE * INPUT_MAT_COUNT + i * MATRIX_LINE + j)
#define P_MAT(offset, i, j) (MATRIX_SIZE * offset + i * MATRIX_LINE + j)

#define P_TYPE(p) ((double *)p)

typedef unsigned int size_t;

void m_func(void * p_data)
{
    for(size_t i = 0; i < MATRIX_LINE; i++)
    {
        for(size_t j = 0; j < MATRIX_LINE; j++)
        {
            for(size_t k = 0; k < MATRIX_LINE; k++)
            {
                P_TYPE(p_data)[P_MAT(2, i, j)] += P_TYPE(p_data)[P_MAT(0, i, k)] * P_TYPE(p_data)[P_MAT(1, k, j)];
            }
        }
    }
}