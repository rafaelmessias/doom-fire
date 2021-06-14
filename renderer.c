// GLEW must always come before SDL_opengl
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"
#include <math.h>
#include "renderer.h"


SDL_Window *Window;
GLuint Vbo, Vao, ShaderProg;
unsigned char pal_idx[WIDTH][HEIGHT];
unsigned char data[WIDTH][HEIGHT][3];
unsigned char (*palette)[3];

GLchar* vertexSrc = \
    "#version 150\n\
    in vec3 in_Position;\
    in vec2 in_Tex;\
    out vec2 texCoord;\
    void main(void) {\
        texCoord = in_Tex;\
        gl_Position = vec4(in_Position, 1.0);\
    }";

GLchar* fragSrc = \
    "#version 150\n\
    precision highp float;\
    in vec2 texCoord;\
    out vec4 fragColor;\
    uniform sampler2D ourTexture;\
    void main(void) {\
        fragColor = texture(ourTexture, texCoord);\
    }";


void initSystem()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    
    Window = SDL_CreateWindow(
        "Doom Fire PSX", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        2 * WIDTH,
        2 * HEIGHT, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext *context = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, context);
    // Without this, on OSX, we get a segfault...!
    glewExperimental = GL_TRUE; 
    glewInit();

    SDL_GL_SetSwapInterval(1);
    glClearColor(0, 0, 0, 1);

    // Setup VAO here
    glGenBuffers(1, &Vbo);
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);    
	glGenVertexArrays(1, &Vao);
    glBindVertexArray(Vao);
        int stride = 5 * sizeof(GL_FLOAT);
        const void *texOffset = (const void *)(3 * sizeof(GL_FLOAT));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0); 
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, texOffset); 
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);   

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&vertexSrc, 0);
    glCompileShader(vertexShader);

    int IsCompiled_VS, maxLength;
    char *vertexInfoLog;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if(IsCompiled_VS == GL_FALSE)
    {
       glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

       /* The maxLength includes the NULL character */
       vertexInfoLog = (char *)malloc(maxLength);

       glGetShaderInfoLog(vertexShader, maxLength, &maxLength, vertexInfoLog);

       /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
       printf("%s", vertexInfoLog);
       /* In this simple program, we'll just leave */
       free(vertexInfoLog);
       exit(-1);
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, (const GLchar**)&fragSrc, 0);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if(IsCompiled_VS == GL_FALSE)
    {
       glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &maxLength);

       /* The maxLength includes the NULL character */
       vertexInfoLog = (char *)malloc(maxLength);

       glGetShaderInfoLog(fragShader, maxLength, &maxLength, vertexInfoLog);

       /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
       printf("%s", vertexInfoLog);
       /* In this simple program, we'll just leave */
       free(vertexInfoLog);
       exit(-1);
    }

    ShaderProg = glCreateProgram();
    glAttachShader(ShaderProg, vertexShader);
    glAttachShader(ShaderProg, fragShader);
    // This connects VAO position 0 (set above) to the shader variable in_Position
    glBindAttribLocation(ShaderProg, 0, "in_Position");
    glBindAttribLocation(ShaderProg, 1, "in_Tex");
    glLinkProgram(ShaderProg);
    glUseProgram(ShaderProg);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // This is needed in order to handle gl_PointCoord in the fragment shader
    // NOTE This is always on in OpenGL 3.2+
    // glEnable(GL_POINT_SPRITE);

    // This is needed for alpha
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);
    
    // This is basically supersampling (all fragments are multisampled). Useful if you are discarding fragments.
    glEnable(GL_SAMPLE_SHADING);
    glMinSampleShading(1.0);

    // NOTE This never seem to work for me.
    // glEnable(GL_POINT_SMOOTH);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load and generate the texture
    // int WIDTH, HEIGHT, nrChannels;
    // unsigned char *data = stbi_load("container.jpg", &WIDTH, &HEIGHT, &nrChannels, 0);
    
        // glGenerateMipmap(GL_TEXTURE_2D);
    // }
    // else
    // {
    //     printf("Failed to load texture\n");
    // }
    // stbi_image_free(data);

    float vertices[] =
    {
        -1.0, -1.0, 0.0,    0, 0,
         1.0, -1.0, 0.0,    1, 0,
         1.0,  1.0, 0.0,    1, 1,
        -1.0,  1.0, 0.0,    0, 1,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    memset(pal_idx, 0, WIDTH * HEIGHT);
    memset(data, 0, WIDTH * HEIGHT * 3);

    // palette = malloc(37 * 3);
    // memcpy(palette, rgbs, 37 * 3);
}

void setPalette(unsigned char srcPal[][3], int len)
{
    if (palette != NULL)
        free(palette);
    palette = (unsigned char (*)[3])malloc(len * 3);
    memcpy(palette, srcPal, len * 3);
}

void startLoop(void (*renderFrame)(void))
{
    int quit = 0;
    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderFrame();

        // unsigned char (*palette)[3] = (unsigned char (*)[3])rgbs;

        // Transfer palette to framebuffer
        for (int i = 0; i < HEIGHT; ++i)
        {
            for (int j = 0; j < WIDTH; ++j)
            {
                data[i][j][0] = palette[pal_idx[i][j]][0];
                data[i][j][1] = palette[pal_idx[i][j]][1];
                data[i][j][2] = palette[pal_idx[i][j]][2];
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        glBindVertexArray(Vao);    
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        SDL_GL_SwapWindow(Window);

        // Without event polling, in OSX the window does not open...!
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) 
        {
            if (e.type == SDL_QUIT) 
            {
                quit = 1;
            }
            if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;
                }
            }
        }        
    }
    while(!quit);
}

// int main(int argc, char *argv[])
// {
//     initSystem();

//     for (int j = 0; j < WIDTH; ++j)
//     {
//         pal_idx[0][j] = 36;
//     }

//     startLoop(__renderFrame);    
// }
