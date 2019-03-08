#include "Parsers.h"
#include <fstream>
#include "extern.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

#include <unordered_map>

void split(std::string to_split, std::string delim, std::vector<std::string>& result) {
	size_t last_offset = 0;
	while (true) {
		//find first delim
		size_t offset = to_split.find_first_of(delim, last_offset); 
		result.push_back(to_split.substr(last_offset, offset - last_offset));
		if (offset == std::string::npos) // if at end of string
			break;
		else //otherwise continue
			last_offset = offset + 1;
	}
}

bool Parsers::parseMTL(std::string path, std::string filename, std::vector<Material>& materials, GLuint shader_id) {
    
    //first we sort the path out (because the filename might be a relative path
    //and we need the full path to read any MTL or texture files associated with the OBJ)
    size_t position_last_separator = filename.find_last_of("\\/");
    
    //if position_last_separator != null,
    if (std::string::npos != position_last_separator) {
        path += filename.substr(0, position_last_separator + 1); //add relative path to path, include slash
        filename = filename.substr(position_last_separator + 1); //remove relative path from filename
    }
    
    std::string line;
    std::ifstream file(path + filename);
    if (file.is_open()) {
        
        Material* curr_mat = nullptr;
        
        //parse file line by line
        while (std::getline(file, line)) {
            //split line string
            std::vector<std::string> words;
            split(line, " ", words);
            
            if (words.empty()) continue; //empty line, skip
            if (words[0][0] == '#') continue; //first word starts with #, line is a comment
            
            //create new material, set shader_id and render mode
            if (words[0] == "newmtl") {
                materials.emplace_back();
                curr_mat = &(materials.back());
                curr_mat->name = words[1];
                curr_mat->shader_id = shader_id;
            }
            if (words[0] == "Ka") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->ambient = lm::vec3((float)std::atof(words[1].c_str()),
                                             (float)std::atof(words[2].c_str()),
                                             (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Kd") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->diffuse = lm::vec3((float)std::atof(words[1].c_str()),
                                             (float)std::atof(words[2].c_str()),
                                             (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Ks") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular = lm::vec3((float)std::atof(words[1].c_str()),
                                              (float)std::atof(words[2].c_str()),
                                              (float)std::atof(words[3].c_str()));
            }
            if (words[0] == "Ns") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular_gloss = (float)std::atof(words[1].c_str());
            }
            if (words[0] == "map_Kd") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->diffuse_map = parseTexture(path + words[1]);
            }
            if (words[0] == "map_Bump") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->normal_map = parseTexture(path + words[1]);
            }
            if (words[0] == "map_Ks") {
                if (!curr_mat) { std::cerr << "ERROR: MTL file is bad, material not initialized;\n"; continue; }
                curr_mat->specular_map = parseTexture(path + words[1]);
            }
        }
        file.close();
        return true;
    }
    std::cerr << "ERROR: Could read MTL file path, name: " << path << ", " << filename << std::endl;
    return false;
}

//parses a wavefront object into passed arrays
bool Parsers::parseOBJ(std::string filename, std::vector<float>& vertices, std::vector<float>& uvs, std::vector<float>& normals, std::vector<unsigned int>& indices) {
    
	std::string line;
	std::ifstream file(filename);
	if (file.is_open())
	{
		//declare containers for temporary and final attributes
		std::vector<lm::vec3> temp_vertices;
		std::vector<lm::vec2> temp_uvs;
		std::vector<lm::vec3> temp_normals;

		//container to store map for indices
		std::unordered_map<std::string, int> indices_map;
		int next_index = 0; //stores next available index

		//parse file line by line
		while (std::getline(file, line))
		{
			//split line string
			std::vector<std::string> words; 
			split(line, " ", words);
						
			if (words.empty()) continue; //empty line, skip
			if (words[0][0] == '#')	continue; //first word starts with #, line is a comment

			if (words[0] == "v") { //line contains vertex data
				//read words to floats
				int wn = 1;
				if (words[1] == "")
					wn = 2;

				lm::vec3 pos( (float)atof( words[wn].c_str() ),
							  (float)atof( words[wn+1].c_str() ),
							  (float)atof( words[wn+2].c_str() ) );
				//add to temporary vector of positions
				temp_vertices.push_back(pos);
			}
			if (words[0] == "vt") { //line contains texture data
				//read words to floats
				lm::vec2 tex( (float)atof(words[1].c_str() ),
							  (float)atof(words[2].c_str() ) );
				//add to temporary vector of texture coords
				temp_uvs.push_back(tex);
			}
			if (words[0] == "vn") { //line contains vertex data
				//read words to floats
				lm::vec3 norm( (float)atof(words[1].c_str() ),
							   (float)atof(words[2].c_str() ),
							   (float)atof(words[3].c_str() ) );
				//add to temporary vector of normals
				temp_normals.push_back(norm);
			}

			//line contains face-vertex data
			if (words[0] == "f") {
				if (words.size() < 4) continue; // faces with fewer than 3 vertices??!

				bool quad_face = false; //boolean to help us deal with quad faces

				std::vector<std::string> nums; // container used for split indices
				//for each face vertex
				for (int i = 1; i < words.size(); i++) {

					if (words[i] == "")
						continue;
					//check if face vertex has already been indexed
					if (indices_map.count(words[i]) == 0) {
					
						//if not, start by getting all indices
						nums.clear();
						split(words[i], "/", nums);
						int v_ind = atoi(nums[0].c_str()) - 1; //subtract 1 to convert to 0-based array!
						int t_ind = atoi(nums[1].c_str()) - 1;
						int n_ind = atoi(nums[2].c_str()) - 1;

						//add vertices to final arrays of floats
						for (int j = 0; j < 3; j++)
							vertices.push_back(temp_vertices[v_ind].value_[j]);
						for (int j = 0; j < 2; j++)
							uvs.push_back(temp_uvs[t_ind].value_[j]);
						for (int j = 0; j < 3; j++)
							normals.push_back(temp_normals[n_ind].value_[j]);
						
						//add an index to final array
						indices.push_back(next_index);

						//record that this index is used for this face vertex
						indices_map[words[i]] = next_index;

						//increment index
						next_index++;
					}
					else {
						//face vertex was already added to final arrays
						//so search for its stored index
						int the_index = indices_map.at(words[i]); //safe to use as we know it exists
						//add it to final index array
						indices.push_back(the_index);
					}

					//***CHECK FOR QUAD FACES****
					//If the face is a quads (i.e. words.size() == 5), we need to create two triangles
					//out of the quad. We have already created one triangle with words[1], [2] and [3]
					//now we make another with [4], [1] and [3]. 
					if (i == 4 ) { 
						//face-vertex 4 is already added, so we search for indices of 1 and 3 and add them
						int index_1 = indices_map.at(words[1]);
						indices.push_back(index_1);
						int index_3 = indices_map.at(words[3]);
						indices.push_back(index_3);
					}

				}

			}

		}
		file.close();
		return true;
	}
	return false;
}

//parses a wavefront object into passed arrays
int Parsers::parseOBJ_multi(std::string filename, std::vector<Geometry>& geometries, std::vector<Material>& materials) {
    
    std::vector<GLfloat> vertices, uvs, normals;
    std::vector<GLuint> indices;
    
    std::string path;
    size_t position_last_separator = filename.find_last_of("\\/");
    if (std::string::npos != position_last_separator) {
        path += filename.substr(0, position_last_separator + 1);
        filename = filename.substr(position_last_separator + 1);
    }
    
    
    std::string line;
    std::ifstream file(path + filename);
    if (file.is_open())
    {
        //declare containers for temporary and final attributes
        std::vector<lm::vec3> temp_vertices;
        std::vector<lm::vec2> temp_uvs;
        std::vector<lm::vec3> temp_normals;
        
        //container to store map for indices
        std::unordered_map<std::string, int> indices_map;
        int next_index = 0; //stores next available index
        
        //current material id allows us to store material ids for sub-materials
        //it starts at -1, we set it as soon as we see a usemtl line
        int current_material_id = -1;
        bool usemtl = false; // used to say whether obj uses mtl or not
        
        //create 'empty' geometry
        geometries.emplace_back();
        Geometry* current_geometry = &(geometries.back());
        
        
        //parse file line by line
        while (std::getline(file, line))
        {
            //split line string
            std::vector<std::string> words;
            split(line, " ", words);
            
            if (words.empty()) continue; //empty line, skip
            if (words[0][0] == '#')    continue; //first word starts with #, line is a comment
            
            if (words[0] == "v") { //line contains vertex data
                //read words to floats
                lm::vec3 pos( (float)atof( words[1].c_str() ),
                             (float)atof( words[2].c_str() ),
                             (float)atof( words[3].c_str() ) );
                //add to temporary vector of positions
                temp_vertices.push_back(pos);
            }
            if (words[0] == "vt") { //line contains texture data
                //read words to floats
                lm::vec2 tex( (float)atof(words[1].c_str() ),
                             (float)atof(words[2].c_str() ) );
                //add to temporary vector of texture coords
                temp_uvs.push_back(tex);
            }
            if (words[0] == "vn") { //line contains vertex data
                //read words to floats
                lm::vec3 norm( (float)atof(words[1].c_str() ),
                              (float)atof(words[2].c_str() ),
                              (float)atof(words[3].c_str() ) );
                //add to temporary vector of normals
                temp_normals.push_back(norm);
            }
            if (words[0] == "f") {//line contains face-vertex data
                if (words.size() < 4) continue; // faces with fewer than 3 vertices??!
                
                bool quad_face = false; //boolean to help us deal with quad faces
                
                std::vector<std::string> nums; // container used for split indices
                //for each face vertex
                for (int i = 1; i < words.size(); i++) {
                    
                    //check if face vertex has already been indexed
                    if (indices_map.count(words[i]) == 0) {
                        
                        //if not, start by getting all indices
                        nums.clear();
                        split(words[i], "/", nums);
                        int v_ind = atoi(nums[0].c_str()) - 1; //subtract 1 to convert to 0-based array!
                        int t_ind = atoi(nums[1].c_str()) - 1;
                        int n_ind = atoi(nums[2].c_str()) - 1;
                        
                        //add vertices to final arrays of floats
                        for (int j = 0; j < 3; j++)
                            vertices.push_back(temp_vertices[v_ind].value_[j]);
                        for (int j = 0; j < 2; j++)
                            uvs.push_back(temp_uvs[t_ind].value_[j]);
                        for (int j = 0; j < 3; j++)
                            normals.push_back(temp_normals[n_ind].value_[j]);
                        
                        //add an index to final array
                        indices.push_back(next_index);
                        
                        //record that this index is used for this face vertex
                        indices_map[words[i]] = next_index;
                        
                        //increment index
                        next_index++;
                    }
                    else {
                        //face vertex was already added to final arrays
                        //so search for its stored index
                        int the_index = indices_map.at(words[i]); //safe to use as we know it exists
                        //add it to final index array
                        indices.push_back(the_index);
                    }
                    
                    //***CHECK FOR QUAD FACES****
                    //If the face is a quads (i.e. words.size() == 5), we need to create two triangles
                    //out of the quad. We have already created one triangle with words[1], [2] and [3]
                    //now we make another with [4], [1] and [3].
                    if (i == 4 ) {
                        //face-vertex 4 is already added, so we search for indices of 1 and 3 and add them
                        int index_1 = indices_map.at(words[1]);
                        indices.push_back(index_1);
                        int index_3 = indices_map.at(words[3]);
                        indices.push_back(index_3);
                    }
                    
                }
                
            }
            if (words[0] == "usemtl") { //
                //we create material sets at the *end* of a list of faces
                //so if this is the first time we see this keyword, we do nothing
                
                if (!usemtl) // first time, do nothing
                    usemtl = true;
                else //subsequent times, create material set for previous indices
                    current_geometry->createMaterialSet((int)indices.size()/3, current_material_id);
                
                //search materials array for mat with name
                //if not exists, current_material_id stays as -1
                for (int i = 0; i < materials.size(); i++){
                    if (materials[i].name == words[1]){
                        current_material_id = i;
                        break;
                    }
                    
                }
            }
            
            
        }
        file.close();
        
        //create vertex arrays
        current_geometry->createVertexArrays(vertices, uvs, normals, indices);
        //close final (or only) material set
        current_geometry->createMaterialSet((int)indices.size()/3, current_material_id);
        
        //return index of new geometry in the geometries array
        return (int)geometries.size() - 1;
    }
    return -1;
}

// load uncompressed RGB targa file into an OpenGL texture
GLint Parsers::parseTexture(std::string filename,
                            ImageData* image_data,
                            bool keep_data) {
    
	std::string str = filename;
	std::string ext = str.substr(str.size() - 4, 4);


	GLuint texture_id;

	if (ext == ".tga" || ext == ".TGA")
	{
		TGAInfo* tgainfo = loadTGA(filename);
		if (tgainfo == NULL) {
			std::cerr << "ERROR: Could not load TGA file" << std::endl;
			return false;
		}

		//generate new openGL texture and bind it (tell openGL we want to do stuff with it)
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id); //we are making a regular 2D texture

												  //screen pixels will almost certainly not be same as texture pixels, so we need to
												  //set some parameters regarding the filter we use to deal with these cases
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	//set the mag filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //set the min filter
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4); //use anisotropic filtering

																		  //this is function that actually loads texture data into OpenGL
		glTexImage2D(GL_TEXTURE_2D, //the target type, a 2D texture
			0, //the base level-of-detail in the mipmap
			(tgainfo->bpp == 24 ? GL_RGB : GL_RGBA), //specified the color channels for opengl
			tgainfo->width, //the width of the texture
			tgainfo->height, //the height of the texture
			0, //border - must always be 0
			(tgainfo->bpp == 24 ? GL_BGR : GL_BGRA), //the format of the incoming data
			GL_UNSIGNED_BYTE, //the type of the incoming data
			tgainfo->data); // a pointer to the incoming data

        //we want to use mipmaps
		glGenerateMipmap(GL_TEXTURE_2D);

        //clean up memory if required
        if (!keep_data) {
            delete tgainfo->data;
            delete tgainfo;
        }
        else {
            //keep image data for use in engine
            image_data->data = tgainfo->data;
            image_data->width = tgainfo->width;
            image_data->height = tgainfo->height;
            image_data->bytes_pp = tgainfo->bpp / 8;
            //delete tgainfo;
        }
		return texture_id;
	}
	else {
		std::cerr << "ERROR: No extension or extension not supported" << std::endl;
		return -1;
	}
}

// this reader supports only uncompressed RGB targa files with no colour table
TGAInfo* Parsers::loadTGA(std::string filename)
{
	//the TGA header is 18 bytes long. The first 12 bytes are for specifying the compression
	//and various fields that are very infrequently used, and hence are usually 0.
	//for this limited file parser, we start by reading the first 12 bytes and compare
	//them against the pattern that identifies the file a simple, uncompressed RGB file.
	//more info about the TGA format cane be found at http://www.paulbourke.net/dataformats/tga/

	char TGA_uncompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	char TGA_compare[12];
	char info_header[6];
	GLuint bytes_per_pixel;
	GLuint image_size;

	//open file
	std::ifstream file(filename, std::ios::binary);

	//read first 12 bytes
	file.read(&TGA_compare[0], 12);
	std::streamsize read_header_12 = file.gcount();
	//compare to check that file in uncompressed (or not corrupted)
	int header_compare = memcmp(TGA_uncompressed, TGA_compare, sizeof(TGA_uncompressed));
	if (read_header_12 != sizeof(TGA_compare) || header_compare != 0) {
		std::cerr << "ERROR: TGA file is not in correct format or corrupted: " << filename << std::endl;
		file.close();
		return nullptr;
	}

	//read in next 6 bytes, which contain 'important' bit of header
	file.read(&info_header[0], 6);

	TGAInfo* tgainfo = new TGAInfo;

	tgainfo->width = info_header[1] * 256 + info_header[0]; //width is stored in first two bytes of info_header
	tgainfo->height = info_header[3] * 256 + info_header[2]; //height is stored in next two bytes of info_header

	if (tgainfo->width <= 0 || tgainfo->height <= 0 || (info_header[4] != 24 && info_header[4] != 32)) {
		file.close();
		delete tgainfo;
		std::cerr << "ERROR: TGA file is not 24 or 32 bits, or has no width or height: " << filename << std::endl;
		return NULL;
	}

	//calculate bytes per pixel and then total image size in bytes
	tgainfo->bpp = info_header[4];
	bytes_per_pixel = tgainfo->bpp / 8;
	image_size = tgainfo->width * tgainfo->height * bytes_per_pixel;

	//reserve memory for the image data
	tgainfo->data = (GLubyte*)malloc(image_size);

	//read data into memory
	file.read((char*)tgainfo->data, image_size);
	std::streamsize image_read_size = file.gcount();

	//check it has been read correctly
	if (image_read_size != image_size) {
		if (tgainfo->data != NULL)
			free(tgainfo->data);
		file.close();
		std::cerr << "ERROR: Could not read tga data: " << filename << std::endl;
		delete tgainfo;
		return NULL;
	}

	file.close();

	return tgainfo;
}

GLuint Parsers::parseCubemap(std::vector<std::string>& faces) {
    
    TGAInfo* tgainfo0 = loadTGA(faces[0]);
    TGAInfo* tgainfo1 = loadTGA(faces[1]);
    TGAInfo* tgainfo2 = loadTGA(faces[2]);
    TGAInfo* tgainfo3 = loadTGA(faces[3]);
    TGAInfo* tgainfo4 = loadTGA(faces[4]);
    TGAInfo* tgainfo5 = loadTGA(faces[5]);
    
    //set the member variables for easy access
    GLsizei width = (GLsizei)tgainfo0->width;
    GLsizei height = (GLsizei)tgainfo0->height;
    
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    
    //Define all 6 faces
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo0->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo1->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo2->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo3->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo4->data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, tgainfo5->data);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 10);
    
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    
    //clean up memory
    delete tgainfo0->data; delete tgainfo0;
    delete tgainfo1->data; delete tgainfo1;
    delete tgainfo2->data; delete tgainfo2;
    delete tgainfo3->data; delete tgainfo3;
    delete tgainfo4->data; delete tgainfo4;
    delete tgainfo5->data; delete tgainfo5;
    
    
    
    return texture_id;
}

bool Parsers::parseJSONLevel(std::string filename,
                             GraphicsSystem& graphics_system, ControlSystem& control_system) {
    //read json file and stream it into a rapidjson document
    //see http://rapidjson.org/md_doc_stream.html
    std::ifstream json_file(filename);
    rapidjson::IStreamWrapper json_stream(json_file);
    rapidjson::Document json;
    json.ParseStream(json_stream);
    //check if its valid JSON
    if (json.HasParseError()) { std::cerr << "JSON format is not valid!" << std::endl;return false; }
    //check if its a valid scene file
    if (!json.HasMember("scene")) { std::cerr << "JSON file is incomplete! Needs entry: scene" << std::endl; return false; }
    if (!json.HasMember("directory")) { std::cerr << "JSON file is incomplete! Needs entry: directory" << std::endl; return false; }
    if (!json.HasMember("textures")) { std::cerr << "JSON file is incomplete! Needs entry: textures" << std::endl; return false; }
    if (!json.HasMember("materials")) { std::cerr << "JSON file is incomplete! Needs entry: materials" << std::endl; return false; }
    if (!json.HasMember("lights")) { std::cerr << "JSON file is incomplete! Needs entry: lights" << std::endl; return false; }
    if (!json.HasMember("entities")) { std::cerr << "JSON file is incomplete! Needs entry: entities" << std::endl; return false; }
    if (!json.HasMember("shaders")) { std::cerr << "JSON file is incomplete! Needs entry: shaders" << std::endl; return false; }
    
    
    printf("Parsing Scene Name = %s\n", json["scene"].GetString());
    
    std::string data_dir = json["directory"].GetString();
    
    //dictionaries
    std::unordered_map<std::string, int> geometries;
    std::unordered_map<std::string, GLuint> textures;
    std::unordered_map<std::string, int> materials;
    std::unordered_map<std::string, int> shaders;
    std::unordered_map<std::string, std::string> child_parent;
    
    //geometries
    for (rapidjson::SizeType i = 0; i < json["geometries"].Size(); i++) {
        //get values from json
        std::string name = json["geometries"][i]["name"].GetString();
        std::string file = json["geometries"][i]["file"].GetString();
        //load geometry
        int geom_id = graphics_system.createGeometryFromFile(data_dir + file);
        //add to dictionary
        geometries[name] = geom_id;
    }
    
    //shaders
    for (rapidjson::SizeType i = 0; i < json["shaders"].Size(); i++) {
        //get values from json
        std::string name = json["shaders"][i]["name"].GetString();
        std::string vertex = json["shaders"][i]["vertex"].GetString();
        std::string fragment = json["shaders"][i]["fragment"].GetString();
        //load shader
        Shader* new_shader = graphics_system.loadShader(vertex, fragment);
        new_shader->name = name;
        shaders[name] = new_shader->program;
        
    }
    
    //cameras
	if (json.HasMember("cameras")) {
		for (rapidjson::SizeType i = 0; i < json["cameras"].Size(); i++) {
			const std::string name = json["cameras"][i]["name"].GetString();
			const std::string type = json["cameras"][i]["name"].GetString();
			const std::string movement = json["cameras"][i]["movement"].GetString();
			auto& jp = json["cameras"][i]["position"];
			auto& jd = json["cameras"][i]["direction"];
			const float fov = json["cameras"][i]["fov"].GetFloat();
			const float near = json["cameras"][i]["near"].GetFloat();
			const float far = json["cameras"][i]["far"].GetFloat();

			int vp_w, vp_h; //get viewport dims from graphics system
			graphics_system.getMainViewport(vp_w, vp_h);


			if (movement == "free") {
				int ent_player = ECS.createEntity("PlayerFree");
				Camera& player_cam = ECS.createComponentForEntity<Camera>(ent_player);
				lm::vec3 the_position(jp[0].GetFloat(), jp[1].GetFloat(), jp[2].GetFloat());
				ECS.getComponentFromEntity<Transform>(ent_player).translate(the_position);
				player_cam.position = the_position;
				player_cam.forward = lm::vec3(jd[0].GetFloat(), jd[1].GetFloat(), jd[2].GetFloat());
				player_cam.setPerspective(fov*DEG2RAD, (float)vp_w / (float)vp_h, near, far);
				ECS.main_camera = ECS.getComponentID<Camera>(ent_player);
				control_system.control_type = ControlTypeFree;
			}
		}
	}
    
    //textures
    for (rapidjson::SizeType i = 0; i < json["textures"].Size(); i++) {
        //get values from json
        std::string name = json["textures"][i]["name"].GetString();
        
        //predeclare id
        GLuint tex_id = 0;
        
        //check if its an environment
        if (json["textures"][i].HasMember("files")) {
            std::vector<std::string> cube_faces{
                data_dir + json["textures"][i]["files"][0].GetString(),
                data_dir + json["textures"][i]["files"][1].GetString(),
                data_dir + json["textures"][i]["files"][2].GetString(),
                data_dir + json["textures"][i]["files"][3].GetString(),
                data_dir + json["textures"][i]["files"][4].GetString(),
                data_dir + json["textures"][i]["files"][5].GetString()
            };
            tex_id = parseCubemap(cube_faces);
        }
        else {
            //else it's a regular texture
            std::string file = json["textures"][i]["file"].GetString();
            //load texture
            tex_id = parseTexture(data_dir + file);
        }
        //add to dictionary
        textures[name] = tex_id;
    }
    
    //environment
    if (json.HasMember("environment")) {
        //get values from json
        std::string texture = json["environment"]["texture"].GetString();
        std::string geometry = json["environment"]["geometry"].GetString();
        std::string shader = json["environment"]["shader"].GetString();
        graphics_system.setEnvironment(textures[texture], geometries[geometry], shaders[shader]);
    }
    
    //materials
    for (rapidjson::SizeType i = 0; i < json["materials"].Size(); i++) {
        //get values from json
        std::string name = json["materials"][i]["name"].GetString();
        
        //create material
        int mat_id = graphics_system.createMaterial();
        
        //shader_id is mandatory
        graphics_system.getMaterial(mat_id).shader_id = shaders[json["materials"][i]["shader"].GetString()];
        
        //optional properties
        
        //diffuse texture
        if (json["materials"][i].HasMember("diffuse_map")) {
            std::string diffuse = json["materials"][i]["diffuse_map"].GetString();
            graphics_system.getMaterial(mat_id).diffuse_map = textures[diffuse]; //assign texture id from material
        }
        
        //diffuse
        if (json["materials"][i].HasMember("diffuse")) {
            auto& json_spec = json["materials"][i]["diffuse"];
            graphics_system.getMaterial(mat_id).diffuse = lm::vec3(json_spec[0].GetFloat(), json_spec[1].GetFloat(), json_spec[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).diffuse = lm::vec3(1, 1, 1); //white diffuse
        
        
        //specular
        if (json["materials"][i].HasMember("specular")) {
            auto& json_spec = json["materials"][i]["specular"];
            graphics_system.getMaterial(mat_id).specular = lm::vec3(json_spec[0].GetFloat(), json_spec[1].GetFloat(), json_spec[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).specular = lm::vec3(0, 0, 0); //no specular
        
        //ambient
        if (json["materials"][i].HasMember("ambient")) {
            auto& json_ambient = json["materials"][i]["ambient"];
            graphics_system.getMaterial(mat_id).ambient = lm::vec3(json_ambient[0].GetFloat(), json_ambient[1].GetFloat(), json_ambient[2].GetFloat());
        }
        else
            graphics_system.getMaterial(mat_id).ambient = lm::vec3(0.1f, 0.1f, 0.1f); //no specular
        
        //reflection
        if (json["materials"][i].HasMember("cube_map")) {
            std::string cube_map = json["materials"][i]["cube_map"].GetString();
            graphics_system.getMaterial(mat_id).cube_map = textures[cube_map];
        }
        
        //add to dictionary
        materials[name] = mat_id;
    }
    
	//lights
	for (rapidjson::SizeType i = 0; i < json["lights"].Size(); i++) {
		std::string light_name = json["lights"][i]["name"].GetString();
		std::string light_type = json["lights"][i]["type"].GetString();

		int ent_light = ECS.createEntity(light_name);
		ECS.createComponentForEntity<Light>(ent_light);

		auto& l = ECS.getComponentFromEntity<Light>(ent_light);

		//set type
		if (light_type == "directional") l.type = 0;
		if (light_type == "point") l.type = 1;
		if (light_type == "spot") l.type = 2;

		//color
		if (json["lights"][i].HasMember("color")) {
			auto json_lc = json["lights"][i]["color"].GetArray();
			l.color = lm::vec3(json_lc[0].GetFloat(), json_lc[1].GetFloat(), json_lc[2].GetFloat());
		}
		//transform
		if (json["lights"][i].HasMember("position")) {
			auto json_lp = json["lights"][i]["position"].GetArray();
			ECS.getComponentFromEntity<Transform>(ent_light).translate(json_lp[0].GetFloat(), json_lp[1].GetFloat(), json_lp[2].GetFloat());
		}
		//direction
		if (json["lights"][i].HasMember("direction")) {
			auto json_ld = json["lights"][i]["direction"].GetArray();
			l.direction = lm::vec3(json_ld[0].GetFloat(), json_ld[1].GetFloat(), json_ld[2].GetFloat());
		}
		//attenuation
		if (json["lights"][i].HasMember("linear_att"))
			l.linear_att = json["lights"][i]["linear_att"].GetFloat();
		if (json["lights"][i].HasMember("quadratic_att"))
			l.quadratic_att = json["lights"][i]["quadratic_att"].GetFloat();
		//spotlight params
		if (json["lights"][i].HasMember("spot_inner"))
			l.spot_inner = json["lights"][i]["spot_inner"].GetFloat();
		if (json["lights"][i].HasMember("spot_outer"))
			l.spot_outer = json["lights"][i]["spot_outer"].GetFloat();
	}
    
    //entities
    for (rapidjson::SizeType i = 0; i < json["entities"].Size(); i++) {
        
        //json for entity
        auto& json_ent = json["entities"][i];
        
        //get name
        std::string json_name = "";
        if (json_ent.HasMember("name"))
            json_name = json_ent["name"].GetString();
        
        //get geometry and material ids - obligatory fields
        std::string json_geometry = json_ent["geometry"].GetString();
        std::string json_material = json_ent["material"].GetString();
        
        //transform - obligatory field
        auto jt = json_ent["transform"]["translate"].GetArray();
        auto jr = json_ent["transform"]["rotate"].GetArray();
        auto js = json_ent["transform"]["scale"].GetArray();
        
        //create entity
        int ent_id = ECS.createEntity(json_name);
        Mesh& ent_mesh = ECS.createComponentForEntity<Mesh>(ent_id);
        ent_mesh.geometry = geometries[json_geometry];
        ent_mesh.material = materials[json_material];
        
        //transform
        auto& ent_transform = ECS.getComponentFromEntity<Transform>(ent_id);
        //rotate
        //get rotation euler angles
        lm::vec3 rotate; rotate.x = jr[0].GetFloat(); rotate.y = jr[1].GetFloat(); rotate.z = jr[2].GetFloat();
        //create quaternion from euler angles
        lm::quat qR(rotate.x*DEG2RAD, rotate.y*DEG2RAD, rotate.z*DEG2RAD);
        //create matrix which represents these rotations
        lm::mat4 R; R.makeRotationMatrix(qR);
        //multiply transform by this matrix
        ent_transform.set(ent_transform * R);
        
        //scale
        ent_transform.scaleLocal(js[0].GetFloat(), js[1].GetFloat(), js[2].GetFloat());
        //translate
        ent_transform.translate(jt[0].GetFloat(), jt[1].GetFloat(), jt[2].GetFloat());
        
        if (json_ent["transform"].HasMember("parent")) {
            std::string json_parent = json_ent["transform"]["parent"].GetString();
            if (json_name == "" || json_parent == "") std::cerr << "ERROR: Parser: Either parent or child has no name";
            child_parent[json_name] = json_parent;
        }
        
        
        //optional fields below
        if (json_ent.HasMember("collider")) {
            std::string coll_type = json_ent["collider"]["type"].GetString();
            if (coll_type == "Box") {
                Collider& box_collider = ECS.createComponentForEntity<Collider>(ent_id);
                box_collider.collider_type = ColliderTypeBox;
                
                auto json_col_center = json_ent["collider"]["center"].GetArray();
                box_collider.local_center.x = json_col_center[0].GetFloat();
                box_collider.local_center.y = json_col_center[1].GetFloat();
                box_collider.local_center.z = json_col_center[2].GetFloat();
                
                auto json_col_halfwidth = json_ent["collider"]["halfwidth"].GetArray();
                box_collider.local_halfwidth.x = json_col_halfwidth[0].GetFloat();
                box_collider.local_halfwidth.y = json_col_halfwidth[1].GetFloat();
                box_collider.local_halfwidth.z = json_col_halfwidth[2].GetFloat();
            }
            ///TODO - Ray
        }
    }
    
    //now link hierarchy need to get transform id from parent entity,
    //and link to transform object from child entity
    for (std::pair<std::string, std::string> relationship : child_parent)
    {
        //get parent entity
        int parent_entity_id = ECS.getEntity(relationship.second);
        Entity& parent = ECS.entities[parent_entity_id];
        int parent_transform_id = parent.components[0]; //transform component is always in slot 0
        
        //get child transform
        Transform& transform_child = ECS.getComponentFromEntity<Transform>(relationship.first);
        
        //link child transform with parent id
        transform_child.parent = parent_transform_id;
    }
    
    return true;
}

