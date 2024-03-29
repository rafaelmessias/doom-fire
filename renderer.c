// GLEW must always come before SDL_opengl
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "SDL_endian.h"
#include "renderer.h"


SDL_Window *Window;
GLuint Vbo, Vao, ShaderProg;
byte pal_idx[HEIGHT][WIDTH];
byte data[HEIGHT][WIDTH][3];
byte palette[256][3];

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
    
    Window = SDL_CreateWindow(
        WINDOW_TITLE, 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        SCALE * WIDTH,
        SCALE * HEIGHT, 
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

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
}

void setPalette(byte srcPal[][3], int len)
{
    memcpy(palette, srcPal, len * 3);
}

void startLoop(void (*renderFrame)(void))
{
    int quit = 0;
    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderFrame();

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
