/*
sdl3_cmake_template
Copyright (C) 2026 Matej Smetana

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// Takes in 2D colored vertices and converts them to 3D

struct InputVertex {
	float2 position : TEXCOORD0;
	float4 color : TEXCOORD1;
};

struct OutputVertex {
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
};


OutputVertex main(InputVertex input) {
	OutputVertex output;
	
	output.position = float4(input.position, 0, 1);
	output.color = input.color;
	
    return output;
}
