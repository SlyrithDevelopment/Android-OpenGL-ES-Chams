//
//  Chams.h
//
//  Created by Rev on 4/06/2022.
//

#ifndef CHAMS_H
#define CHAMS_H

#include <fstream>
#include <sstream>
#include <vector>

const char *isUseShader = OBFUSCATE("_BumpMap");
const char *isUniformLocation = OBFUSCATE("glGetUniformLocation");
const char *isDrawElements = OBFUSCATE("glDrawElements");
const char *isDumpLocation = OBFUSCATE("/sdcard/Download/Shaders.txt");
const char *isSymbolError = dlerror();

auto isGLESHandle = dlopen(OBFUSCATE("libGLESv2.so"), RTLD_LAZY);
auto isglGetUniformLocationAddress = dlsym(isGLESHandle, isUniformLocation);
auto isglDrawElementsAddress = dlsym(isGLESHandle, isDrawElements);

bool isDumpToSD = true;
bool isLog = true;

/*
 * dlopen allows us to get the handle of a specified shared object - which is libGLESv2.so in my case.
 * dlsym returns the address of a symbol, from the given handle.
 * dlerror returns a null terminated string of characters that delineates the last error when calling dlopen. For example, if it can't find 'glDrawElements' from the given handle 'libGLESv2.so', it will return an error.
 * Be sure to implement <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/> in the AndroidManifest.XML if you would like to dump the shaders.
*/

string isDump(const char *name)
{
    vector<string> isShaders = {name};

    static string isTotalShaders;

    for (const auto &isAddTo: isShaders)
        isTotalShaders += (isAddTo + "\n"); // Adding whatever is added to the string vector to isTotalShaders, with a new line added per shader.

    return isTotalShaders.c_str();
}

GLint (*old_glGetUniformLocation)(GLuint program, const GLchar *name);
GLint glGetUniformLocation(GLuint program, const GLchar *name) // returns location of a shader/uniform.
{
    if(isLog)
        LOGINFO("Shader: %s Program: %i", name, program);

    if(isDumpToSD)
        ofstream(isDumpLocation) << isDump(name);

    return old_glGetUniformLocation(program, name);
}

/*
 * What is a uniform, and what does glGetUniformLocation do?
 * A uniform is a global shader - meaning that it can be accessed from all scopes. glGetUniformLocation returns the location of a uniform based on the given program, and uniform name.
 * Read more: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
 */

bool isShader() // https://stackoverflow.com/a/62663705/17529905
{
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program); // returns the value of program -> https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGet.xhtml

    return old_glGetUniformLocation(program, isUseShader) != -1;
}

/*
 * This function returns -1 if "name" (isUseShader) does not correspond to an active uniform variable in program (program), if name starts with the reserved prefix "gl_",
 * or if name is associated with an atomic counter or a named uniform block." https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
 */

/*
 * Why != -1?
 * It isn't a requirement to have it. This is just here in case the shader isn't found within the given program, it won't return -1.
 * So, why is it important? Well, since it is a bool, anything that isn't 0 will evaluate to true.
 * In cases that it can't get the shader, and it returns -1, it will enable every shader at once in that specified program.
 * Think of it like this, if you're searching for a shader that doesn't exist in the program, and it returns true, it will "enable" every shader because it's not returning true for any specific shader - as it doesn't exist, and returning -1 evaluates to true.
 * If the shader does exist, it will return the actual id for it (> 0), and make the isShader check specific to it.
 * glGetUniformLocation's purpose is to return the location of a shader/uniform - where returning -1 indicates an issue.
 * I hope that makes sense!
 */

void (*old_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    old_glDrawElements(mode, count, type, indices);

    if(isShader() > 0)
    {
        /* LOGDEBUG("Count: %i", count); By using this, you can get the count from the shader. Logging the count from '_BumpMap' gives you 10716, 1116, 8868, 5394. So, if you were to do
         * if(count == 10716 || count == 1116 || count == 8868 || count == 5394), you wouldn't need to use if(isShader() > 0). */

        
        /* Base-Colour Chams: https://i.imgur.com/Mab5QyF.png
        The code below will always use the same colour on the shader, no matter the depth.

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
        glBlendColor(1.0f, 0.647f, 0.0f, 1.0f); // orange -> 255,165,0 -> r, g, b /255 -> 1.0, 0.647, 0.0

        old_glDrawElements(GL_TRIANGLES, count, type, indices);

        glEnable(GL_DEPTH_TEST); // re-enable depth testing, so that we don't see every shader through walls. */


        /* Visibility-Check Chams: https://i.imgur.com/GRUl5eT.png
        Seeing players through objects works purely because of GL_DEPTH_TEST. When we disable it, it will show our shader through all depths (whether it be walls, or other objects, etc).
        When we give a colour specific to this disabled GL_DEPTH_TEST (let it be green), it will give a green colour to the shader when the shaders depth isn't equal to our shaders depth (not visible).
        When we re-enable it, and use glBlendColor again (red), it will give a specific colour to the shaders that ARE in your current depth - which would be something that is visible by your player.

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
        glBlendColor(0.0f, 1.0f, 0.0f, 1.0f);

        old_glDrawElements(GL_TRIANGLES, count, type, indices);

        glEnable(GL_DEPTH_TEST);

        glBlendColor(1.0f, 0.0f, 0.0f, 1.0f);

        old_glDrawElements(GL_TRIANGLES, count, type, indices); */


        /* Wireframe Chams: https://i.imgur.com/L2FKuKz.png
        To make wireframe chams, all we need to do is specify a line width to use, and then use 'GL_LINES' in our 'mode' parameter. We will also add the depth tests, as usual.
        glDepthFunc with the "GL_LESS" parameter is used to keep rendering the wireframes, even when the object (or part of the object) isn't visible to us as per the depth.

        glDisable(GL_DEPTH_TEST);
        glLineWidth(1.0f);

        old_glDrawElements(GL_LINES, count, type, indices);

        glEnable(GL_DEPTH_TEST);

        glDepthFunc(GL_LESS);

        old_glDrawElements(GL_TRIANGLES, count, type, indices); */


        /* Black Flat Chams: https://i.imgur.com/z2fWmOz.png
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendFunc(GL_ZERO, GL_ZERO);

        old_glDrawElements(GL_TRIANGLES, count, type, indices);

        glEnable(GL_DEPTH_TEST); */


        /* Invisible Chams: https://i.imgur.com/CSnca4n.png
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        glBlendFunc(GL_ZERO, GL_ONE);

        old_glDrawElements(GL_TRIANGLES, count, type, indices);

        glEnable(GL_DEPTH_TEST);

        glDepthFunc(GL_LESS); */


        // Rainbow chams can be easily made with my Rainbow Colour Cycler header; https://github.com/Rev/ImGui-Rainbow-Colour-Cycler
        
        // Interested in creating your own 'style' of chams? Read the documentation!
        // https://docs.microsoft.com/en-us/windows/win32/opengl/gl-functions
        // https://www.khronos.org/
        // https://learnopengl.com/

    }

}

/*
 * What does glDrawElements do?
 * In short, glDrawElements is used to render a set of geometric primitives (triangles, lines, etc - which is shown in the 'mode' parameter).
 * Read more: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glDrawElements.xhtml
 */

void isChams()
{
    if(isSymbolError)
    {
        LOGERROR(OBFUSCATE("Symbol Error: %s"), isSymbolError);
    }

    else
    {
        ARMPatch::hook((void *) isglGetUniformLocationAddress, (void *) &glGetUniformLocation, (void **) &old_glGetUniformLocation);
        ARMPatch::hook((void *) isglDrawElementsAddress, (void *) &glDrawElements, (void **) &old_glDrawElements);
    }
}

#endif
