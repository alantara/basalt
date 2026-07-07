struct VSInput{
  float3 position;
  float3 color;
}

struct VSOutput{
  float4 pos: SV_POSITION;
  float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input){
  VSOutput output;
  output.pos = float4(input.position, 1.0);
  output.color = input.color;
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput input){
  return float4(input.color, 1.0);
}
