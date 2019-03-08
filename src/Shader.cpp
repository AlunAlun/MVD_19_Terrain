#include "Shader.h"
#include <vector>
#include <fstream>
#include <sstream>


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

Shader::Shader() {}

//uniform setters
//int
bool Shader::setUniform(UniformID id, const int data) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform1i(loc, data);
        return true;
    }
    return false;
}
//float
bool Shader::setUniform(UniformID id, const float data) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform1f(loc, data);
        return true;
    }
    return false;
}

//set vec2 array
bool Shader::setUniform(UniformID id, const lm::vec2& data) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform2fv(loc, 1, data.value_);
        return true;
    }
    return false;
}

//set vec3 array
bool Shader::setUniform(UniformID id, const lm::vec3& data) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform3fv(loc, 1, data.value_);
        return true;
    }
    return false;
}

//mat4 array
bool Shader::setUniform(UniformID id, const lm::mat4& data) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, data.m);
        return true;
    }
    return false;
}

//uniform block
bool Shader::setUniformBlock(UniformID id, const int binding_point) {
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniformBlockBinding(program, loc, binding_point);
        return true;
    }
    return false;
}

//texture
bool Shader::setTexture(UniformID id, GLuint tex_id, GLuint unit) {
    //get texture id and bind it
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    // tell sampler which slot its in
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform1i(loc, unit);
        return true;
    }
    return false;
}
//texture cube
bool Shader::setTextureCube(UniformID id, GLuint tex_id, GLuint unit) {
    //get texture id and bind it
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_id);
    // tell sampler which slot its in
    GLint loc = getUniformLocation(id);
    if (loc != -1) {
        glUniform1i(loc, unit);
        return true;
    }
    return false;
}



std::string Shader::readFile(std::string filename) {
	std::ifstream f(filename);
	std::stringstream buffer;
	buffer << f.rdbuf();
	std::string content = buffer.str();
	return content;
}

Shader::Shader(std::string vertSource, std::string fragSource) {
    std::vector<std::string> result = split(fragSource, '/');
    name = result.back();
	std::string vertexShaderSourceCode=readFile(vertSource);
	std::string fragmentShaderSourceCode=readFile(fragSource);
    makeShaderProgram(makeVertexShader(vertexShaderSourceCode.c_str()), makeFragmentShader(fragmentShaderSourceCode.c_str()));
}

GLuint Shader::compileFromStrings(std::string vsh, std::string fsh) {
	makeShaderProgram(makeVertexShader(vsh.c_str()), makeFragmentShader(fsh.c_str()));
	return 1;
}

GLuint Shader::makeVertexShader(const char* shaderSource)
{
    GLuint vertexShaderID=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID,1,(const GLchar**)&shaderSource, NULL);
    glCompileShader(vertexShaderID);
    
    GLint compile=0;
    glGetShaderiv(vertexShaderID,GL_COMPILE_STATUS,&compile);
    
    //we want to see the compile log if we are in debug (to check warnings)
    if (!compile)
    {
        saveShaderInfoLog(vertexShaderID);
        std::cout << "Shader code:\n " << std::endl;
        std::string code = shaderSource;
        std::vector<std::string> lines = split( code, '\n' );
        for( size_t i = 0; i < lines.size(); ++i)
            std::cout << i << "  " << lines[i] << std::endl;
    
    }
    
    
    return vertexShaderID;
}
GLuint Shader::makeFragmentShader(const char* shaderSource)
{
    GLuint fragmentShaderID=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID,1,(const GLchar**)&shaderSource, NULL);
    glCompileShader(fragmentShaderID);
    
    GLint compile=0;
    glGetShaderiv(fragmentShaderID,GL_COMPILE_STATUS,&compile);
    
    //we want to see the compile log if we are in debug (to check warnings)
    if (!compile)
    {
        saveShaderInfoLog(fragmentShaderID);
        std::cout << "Shader code:\n " << std::endl;
        std::string code = shaderSource;
        std::vector<std::string> lines = split( code, '\n' );
        for( size_t i = 0; i < lines.size(); ++i)
            std::cout << i << "  " << lines[i] << std::endl;
        
    }
    
    return fragmentShaderID;
}

void Shader::saveShaderInfoLog(GLuint obj)
{
    int len = 0;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
    
    if (len > 0)
    {
        char* ptr = new char[len+1];
        GLsizei written=0;
        glGetShaderInfoLog(obj, len, &written, ptr);
        ptr[written-1]='\0';
        log.append(ptr);
        delete[] ptr;
        
        printf("LOG **********************************************\n%s\n",log.c_str());
    }
}

void Shader::saveProgramInfoLog(GLuint obj)
{
    int len = 0;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
    
    if (len > 0)
    {
        char* ptr = new char[len+1];
        GLsizei written=0;
        glGetProgramInfoLog(obj, len, &written, ptr);
        ptr[written-1]='\0';
        log.append(ptr);
        delete[] ptr;
        
        printf("LOG **********************************************\n%s\n",log.c_str());
    }
}

void Shader::makeShaderProgram(GLuint vertexShaderID,GLuint fragmentShaderID)
{
    program=glCreateProgram();
    glAttachShader(program, vertexShaderID);
    glAttachShader(program,fragmentShaderID);
    
    glLinkProgram(program);
    GLint link_ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fprintf(stderr, "glLinkProgram:");
        saveProgramInfoLog(program);
    }
    
    //init uniforms
    initUniforms_();
}

GLint Shader::bindAttribute(const char* attribute_name) {
    GLint attribute_ID = glGetAttribLocation(program, attribute_name);
    if (attribute_ID == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }
    else return attribute_ID;
}

//first initializes uniform location vector, then maps uniform locations
//to each id, using the 
void Shader::initUniforms_() {
    
	//initialize uniform location vector to all -1 (not found) 
	uniform_locations_ = std::vector<GLuint>(UNIFORMS_COUNT, -1);

	//iterate map of all possible uniforms, asking shader if it has them
	for (std::pair<std::string, UniformID> element : uniform_string2id_)
	{
		std::string uniform_name = element.first;
		UniformID uniform_id = element.second;
		uniform_locations_[uniform_id] = glGetUniformLocation(program, uniform_name.c_str());
	}
    
    //now do the same for uniform blocks
    for (std::pair<std::string, UniformID> element : uniformblock_string2id_)
    {
        std::string uniform_name = element.first;
        UniformID uniform_id = element.second;
        uniform_locations_[uniform_id] = glGetUniformBlockIndex(program, uniform_name.c_str());
    }
}

//Returns location of uniform with given enum
GLuint Shader::getUniformLocation(UniformID uni_name) {
	return uniform_locations_[uni_name];
}







