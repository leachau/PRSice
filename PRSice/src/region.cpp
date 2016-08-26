/*
 * region.cpp
 *
 *  Created on: 25 Aug 2016
 *      Author: shingwanchoi
 */

#include "../inc/region.hpp"

Region::Region() {
	// TODO Auto-generated constructor stub

}

Region::~Region() {
	// TODO Auto-generated destructor stub
}

#if defined(__LP64__) || defined(_WIN64)
	uint64_t* Region::check(std::string chr, size_t loc){
		uint64_t* res = new uint64_t[(m_region_name.size()+1)/(sizeof(uint64_t)*CHAR_BIT)+1];
		res[0]=1; // base region which contains everything
		for(size_t i = 0; i < m_region_list.size(); ++i){
			while(m_region_list[m_index[i]]){

			}
		}

		return res;
	}
#else
	uint32_t* Region::check(std::string chr, size_t loc){
		uint32_t* res = new uint64_t[(m_region_name.size()+1)/(sizeof(uint32_t)*CHAR_BIT)+1];
		return res;
	}
#endif
void Region::process_bed(const std::vector<std::string> &bed){
	for(size_t i = 0; i < bed.size(); ++i){
		// go through each bed file
		std::ifstream bed_file;
		bool error=false;
		bed_file.open(bed[i].c_str());
		if(!bed_file.is_open()) fprintf(stderr, "WARNING: %s cannot be open. It will be ignored\n", bed[i].c_str());
		else{
			m_region_name.push_back(bed[i]);
			std::vector<boundary > current_region;
			std::string line;
			while(std::getline(bed_file, line)){
				misc::trim(line);
				if(!line.empty()){
					std::vector<std::string> token = misc::split(line);
					if(token.size() < 3){
						fprintf(stderr, "ERROR: %s contain less than 3 column\n", bed[i].c_str());
						fprintf(stderr, "       This file will be ignored\n");
						error=true;
						break;
					}
					int temp = 0;
					size_t start=0, end=0;
					try{
						temp =misc::convert<int>(token[1]);
						if(temp >= 0) start = temp+1;//That's because bed is 0 based
						else{
							fprintf(stderr, "ERROR: Negative Coordinates!\n");
							fprintf(stderr, "       This file will be ignored\n");
							error=true;
							break;
						}

					}
					catch(const std::runtime_error &er){
						fprintf(stderr, "ERROR: Cannot convert some of the coordinates!\n");
						fprintf(stderr, "       This file will be ignored\n");
						error=true;
						break;
					}
					try{
						temp =misc::convert<int>(token[2]); //because bed originally is exclusive
						if(temp >= 0) end = temp;
						else{
							fprintf(stderr, "ERROR: Negative Coordinates!\n");
							fprintf(stderr, "       This file will be ignored\n");
							error=true;
							break;
						}

					}
					catch(const std::runtime_error &er){
						fprintf(stderr, "ERROR: Cannot convert some of the coordinates!\n");
						fprintf(stderr, "       This file will be ignored\n");
						error=true;
						break;
					}
					current_region.push_back(boundary(token[0], start, end));
				}
			}
			if(!error){
				m_region_list.push_back(current_region);
			}
			else{
				m_region_name.pop_back();
			}
			bed_file.close();
		}
	}
}

std::map<std::string, Region::boundary > Region::process_gtf(const std::string &gtf, std::map<std::string, std::string> &id_to_name, bool gen_bed){
	std::map<std::string, boundary > res;
	if(gtf.empty()) return res; // basically return an empty map
	std::ifstream gtf_file;
	gtf_file.open(gtf.c_str());
	if(!gtf_file.is_open()){
		std::string error_message = "Cannot open gtf file: "+gtf;
		throw std::runtime_error(error_message);
	}
	std::string line;
	bool error=false;
	while(std::getline(gtf_file, line)){
		misc::trim(line);
		if(!line.empty()){
			std::vector<std::string> token = misc::split(line, "\t");
			if(token[1].compare("exon")==0 || token[1].compare("gene")==0){
				std::string chr = token[0];
				int temp=0;
				size_t start=0, end=0;
				try{
					temp = misc::convert<int>(token[3]);
					if(temp < 0){
						fprintf(stderr, "ERROR: Negative Coordinate!\n");
						fprintf(stderr, "       Will ignore the gtf file\n");
						error=true;
						break;
					}
					start=temp;

				}
				catch(const std::runtime_error &er){
					error=true;
					fprintf(stderr, "ERROR: Cannot convert some of the coordinates!\n");
					fprintf(stderr, "       Will ignore the gtf file\n");
				}
				try{
					temp = misc::convert<int>(token[4]);
					if(temp < 0){
						fprintf(stderr, "ERROR: Negative Coordinate!\n");
						fprintf(stderr, "       Will ignore the gtf file\n");
						error=true;
							break;
					}
					end=temp;
				}
				catch(const std::runtime_error &er){
					error=true;
					fprintf(stderr, "ERROR: Cannot convert some of the coordinates!\n");
					fprintf(stderr, "       Will ignore the gtf file\n");
				}
				//Now extract the name
				std::vector<std::string> info = misc::split(token[8], ";");
				std::string name="", id="";
				for(size_t i = 0; i < info.size(); ++i){
					// check the name and id
					if(info[i].find("gene_id") != std::string::npos){
						std::vector<std::string> extract = misc::split(info[i]);
						if(extract.size() > 1){
							extract[1].erase(std::remove(extract[1].begin(), extract[1].end(), 'a'), extract[1].end());
							id = extract[1];
						}
					}
					else if(info[i].find("gene_name")!=std::string::npos){
						std::vector<std::string> extract = misc::split(info[i]);
						if(extract.size() > 1){
							extract[1].erase(std::remove(extract[1].begin(), extract[1].end(), 'a'), extract[1].end());
							name = extract[1];
						}

					}
				}
				if(!id.empty()){
					if(id_to_name.find(id)==id_to_name.end()) id_to_name[id]=name;
					else id_to_name[id]=id_to_name[id]+","+name;
				}
				//Now add the information to the map using the id
				if(res.find(id)!=res.end()){
					if(std::get<0>(res[id]).compare(chr)!=0){
						error=true;
						fprintf(stderr, "ERROR: Same gene occur on two separate chromosome!\n");
						fprintf(stderr, "       Will ignore the gtf file\n");
					}
					if(std::get<1>(res[id])> start)std::get<1>(res[id]) = start;
					if(std::get<2>(res[id])< end)std::get<2>(res[id]) = end;
				}
				else{
					res[id]=boundary(chr, start, end);
				}
			}
		}
	}
	if(error){
		res.clear();
		id_to_name.clear();
	}
	else{
		if(gen_bed){
			std::string out_name="";
			std::vector<std::string>token =misc::split(gtf,".");
			for(size_t i = 0; i < token.size()-1; ++i) out_name.append(token[i]+".");
			out_name.append("bed");
			struct stat buffer;
			if(stat (out_name.c_str(), &buffer) == 0){
				// Issue a warning of file exist
				fprintf(stderr, "WARNING: %s exists, will overwrite.\n", out_name.c_str()); //sorry, it is too late, muwhahaha
			}
			std::ofstream out;
			out.open(out_name.c_str());
			for(std::map<std::string, boundary >::iterator iter = res.begin(); iter != res.end(); ++iter){
				out << std::get<0>(iter->second) << "\t" <<std::get<0>(iter->second) << "\t" << std::get<0>(iter->second) << "\t" << iter->first;
				if(id_to_name.find(iter->first)!=id_to_name.end()) out << "\t" << id_to_name[iter->first];
				out << std::endl;
			}
			out.close();
		}
	}
	gtf_file.close();
	return res;
}

void Region::process_msigdb(const std::string &msigdb,
							const std::map<std::string, boundary > &gtf_info,
							const std::map<std::string, std::string> &id_to_name){
	if(msigdb.empty() || gtf_info.size()==0) return; // Got nothing to do
	//Assume format = Name URL Gene
	std::ifstream input;
	input.open(msigdb.c_str());
	if(!input.is_open()) fprintf(stderr, "Cannot open %s. Will skip this file\n", msigdb.c_str());
	else{
		std::string line;
		bool error=false;
		while(std::getline(input, line)){
			misc::trim(line);
			if(!line.empty()){
				std::vector<std::string> token = misc::split(line);
				if(token.size() < 2){ // Will treat the url as gene just in case
					fprintf(stderr, "Each line require at least 2 information\n");
					fprintf(stderr, "%s\n", line.c_str());
				}
				else{
					std::string name = token[0];
					std::vector<boundary> current_region;
					for(size_t i = 1; i < token.size(); ++i){
						if(gtf_info.find(token[i])==gtf_info.end()){
							//Cannot find this gene
							if(gtf_info.find(id_to_name.at(token[i]))!=gtf_info.end()) current_region.push_back(gtf_info.at(id_to_name.at(token[i])));
						}
						else current_region.push_back(gtf_info.at(token[i]));
					}
					m_region_list.push_back(current_region);
					m_region_name.push_back(name);
				}
			}
		}
		input.close();
	}
}

void Region::run(const std::string &gtf, const std::string &msigdb, const std::vector<std::string> &bed, bool gen_bed){
	process_bed(bed);
	std::map<std::string, std::string> id_to_name;
	if(!gtf.empty()){ // without the gtf file, we will not process the msigdb file
		std::map<std::string, boundary > gtf_info;
		try{
			gtf_info=process_gtf(gtf, id_to_name, gen_bed);
		}
		catch(const std::runtime_error &error){
			fprintf(stderr, "ERROR: Cannot process gtf file: %s\n", error.what());
			fprintf(stderr, "       Will not process any of the msigdb items\n");
		}
		if(gtf_info.size() != 0){
			process_msigdb(msigdb, gtf_info, id_to_name);
		}
	}
	size_t size = m_region_name.size();
	if(size==1) fprintf(stderr, "%zu region is included\n", size);
	else if(size>1) fprintf(stderr, "A total of %zu regions are included\n", m_region_name.size());
	m_index = std::vector<size_t>(m_region_name.size());
}
