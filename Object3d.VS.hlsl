struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

struct VertexShadeInput
{
    float32_t4 position : POSITION0;
};

VertexShaderOutput main(VertexShadeInput input)
{
    VertexShaderOutput output;
    output.position = input.position;
    return output;
}