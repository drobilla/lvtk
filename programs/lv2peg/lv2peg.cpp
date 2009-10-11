/****************************************************************************
    
    LV2PEG - LV2 Port Enum Generator

    This program reads an RDF file (in the Turtle syntax) describing
    an LV2 plugin and generates a C header file with data structures
    that contain information about the plugin.
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#include <cfloat>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <redland.h>

#ifndef VERSION
#define VERSION "UNKNOWNVERSION"
#endif


using namespace std;


struct PortInfo {
  PortInfo() 
    : min(-FLT_MAX), 
      max(FLT_MAX), 
      default_value(min),
      toggled(false),
      integer(false),
      logarithmic(false) { }
  string name;
  float min;
  float max;
  float default_value;
  bool toggled;
  bool integer;
  bool logarithmic;
};


/** A template wrapper to handle all the annoying memory management for 
    librdf types. */
template <typename T, void(*F)(T*)>
class RDFPtr {
public:
  /** In the constructor, just take ownership of a T pointer. */
  RDFPtr(T* t) : m_t(t) { }
  /** In the destructor, call the deallocation function. */
  ~RDFPtr() { if (m_t) F(m_t); }
  /** Get the underlying pointer, to pass to librdf functions. */
  T* get() { return m_t; }
  /** Bool conversion operator. */
  operator bool() { return m_t != 0; }
private:
  /** Copying is not allowed, that could cause multiple deletions. */
  RDFPtr(RDFPtr const&) { }
  
  T* m_t;
};


/** Convenient typedef for a RDFPtr instance wrapping librdf_uri. */
typedef RDFPtr<librdf_uri, &librdf_free_uri> RDFUri;

/** Convenient typedef for a RDFPtr instance wrapping librdf_query. */
typedef RDFPtr<librdf_query, &librdf_free_query> RDFQuery;

/** Convenient typedef for a RDFPtr instance wrapping librdf_world. */
typedef RDFPtr<librdf_world, &librdf_free_world> RDFWorld;

/** Convenient typedef for a RDFPtr instance wrapping librdf_model. */
typedef RDFPtr<librdf_model, &librdf_free_model> RDFModel;

/** Convenient typedef for a RDFPtr instance wrapping librdf_parser. */
typedef RDFPtr<librdf_parser, &librdf_free_parser> RDFParser;

/** Convenient typedef for a RDFPtr instance wrapping librdf_storage. */
typedef RDFPtr<librdf_storage, &librdf_free_storage> RDFStorage;

/** Convenient typedef for a RDFPtr instance wrapping librdf_query_results. */
typedef RDFPtr<librdf_query_results, &librdf_free_query_results> RDFResults;


/** Convenience function for parsing a Turtle file without all the librdf
    nastyness. */
librdf_model* parse_file(RDFWorld& world, string const& filename) {
  
  RDFParser parser(librdf_new_parser(world.get(), "turtle", 0, 0));
  if (!parser) {
    cerr<<"Failed to initialise Turtle parser."<<endl;
    return 0;
  }
  
  RDFStorage storage(librdf_new_storage(world.get(), "memory", "storage", 0));
  if (!storage)  {
    cerr<<"Failed to initialise RDF storage."<<endl;
    return 0;
  }
  
  librdf_model* model = librdf_new_model(world.get(), storage.get(), 0);
  if (!model) {
    cerr<<"Failed to initialise RDF data model."<<endl;
    return 0;
  }
  
  RDFUri file_uri(librdf_new_uri_from_filename(world.get(), filename.c_str()));
  if (!file_uri) {
    cerr<<"Failed to create URI from filename."<<endl;
    return 0;
  }
  if (librdf_parser_parse_into_model(parser.get(), file_uri.get(), 0, model))
    cerr<<"Failed to parse the input file."<<endl;
  
  return model;
}


/** Convenience function for running a SPARQL query on a model without all the
    librdf nastyness. */
librdf_query_results* run_query(RDFModel& model, RDFWorld& world,
				string const& query, string const& base = "") {
  RDFUri base_uri(0);
  if (base != "")
    base_uri = 
      librdf_new_uri(world.get(), 
		     reinterpret_cast<unsigned char const*>(base.c_str()));

  RDFQuery rdf_query(librdf_new_query(world.get(), "sparql", 0,
				      reinterpret_cast<unsigned char const*>(query.c_str()),
				      base_uri.get()));
  if (!rdf_query) {
    cerr<<"Failed to initialise SPARQL query."<<endl;
    return 0;
  }
  librdf_query_results* rdf_results =
    librdf_query_execute(rdf_query.get(), model.get());
  if (!rdf_results)
    cerr<<"Failed to execute plugin query."<<endl;
  
  return rdf_results;
}


int main(int argc, char** argv) {
  
  for (int i = 1; i < argc; ++i) {
    if (string(argv[i]) == "-V" || string(argv[i]) == "--version") {
      cout<<"lv2peg "<<VERSION<<" by Lars Luthman <lars.luthman@gmail.com>"
          <<endl;
      return 0;
    }
  }
  
  if (argc < 3) {
    cerr<<"Use like this: lv2peg <input file> <output file> "<<endl;
    return -1;
  }
  
  // initialise librdf
  RDFWorld world (librdf_new_world());
  if (!world) {
    cerr<<"Failed to initialise librdf."<<endl;
    return -1;
  }
  
  // parse turtle file
  RDFModel model(parse_file(world, argv[1]));
  if (!model)
    return -1;

  // find all plugins in the file
  RDFResults 
    results(run_query(model, world,
		      "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
		      "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
		      "SELECT DISTINCT ?plugin, ?pegname WHERE { \n"
		      "?plugin a :Plugin. \n"
		      "?plugin ll:pegName ?pegname. }"));
  if (!results)
    return -1;

  map<string, string> plugins;
  while (!librdf_query_results_finished(results.get())) {
    plugins[(char*)librdf_uri_as_string(librdf_node_get_uri(librdf_query_results_get_binding_value(results.get(), 0)))] = 
      (char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 1));
    librdf_query_results_next(results.get());
  }
  
  // iterate over all plugins
  map<string, map<int, PortInfo> > info;
  map<string, string>::const_iterator plug_iter;
  
  for (plug_iter = plugins.begin(); plug_iter != plugins.end(); ++plug_iter) {
    
    // query the plugin ports
    map<int, PortInfo> ports;
    {
      RDFResults results(run_query(model, world,
				   "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
				   "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
				   "SELECT ?index, ?symbol WHERE { \n"
				   "<>        :port       ?port. \n"
				   "?port     :index      ?index; \n"
				   "          :symbol     ?symbol. }",
				   plug_iter->first));
      if (!results)
	return -1;

      while (!librdf_query_results_finished(results.get())) {
	int port_index = atoi((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 0)));
	if (ports.find(port_index) != ports.end()) {
	  cerr<<"Index "<<port_index<<" is used for more than one port"<<endl;
	  return -1;
	}
	ports[port_index].name = (char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 1));
	librdf_query_results_next(results.get());
      }
      
      // check that the port indices are OK
      map<int, PortInfo>::const_iterator iter;
      int next = 0;
      for (iter = ports.begin(); iter != ports.end(); ++iter) {
	if (iter->first != next) {
	  cerr<<"There was no description of port "<<next
	      <<" in plugin "<<plug_iter->first<<endl;
	  return -1;
	}
	++next;
      }
    }
    
    // get min values
    {
      RDFResults results(run_query(model, world,
				   "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
				   "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
				   "SELECT ?index, ?min WHERE { \n"
				   "<>        :port       ?port. \n"
				   "?port     :index      ?index; \n"
				   "          :minimum    ?min. }",
				   plug_iter->first));
      if (!results)
	return -1;
      
      while (!librdf_query_results_finished(results.get())) {
	int port_index = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 0)));
	ports[port_index].min = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 1)));
	librdf_query_results_next(results.get());
      }
    }
    
    // get max values
    {
      RDFResults results(run_query(model, world,
				   "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
				   "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
				   "SELECT ?index, ?min WHERE { \n"
				   "<>        :port       ?port. \n"
				   "?port     :index      ?index; \n"
				   "          :maximum    ?min. }",
				   plug_iter->first));
      if (!results)
	return -1;
      
      while (!librdf_query_results_finished(results.get())) {
	int port_index = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 0)));
	ports[port_index].max = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 1)));
	librdf_query_results_next(results.get());
      }
    }
    
    // get default values
    {
      RDFResults results(run_query(model, world,
				   "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
				   "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
				   "SELECT ?index, ?min WHERE { \n"
				   "<>        :port       ?port. \n"
				   "?port     :index      ?index; \n"
				   "          :default    ?min. }",
				   plug_iter->first));
      if (!results)
	return -1;
      
      while (!librdf_query_results_finished(results.get())) {
	int port_index = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 0)));
	ports[port_index].default_value = atof((char*)librdf_node_get_literal_value(librdf_query_results_get_binding_value(results.get(), 1)));
	librdf_query_results_next(results.get());
      }
    }
    
    // get port hints
    {
      RDFResults results(run_query(model, world,
				   "PREFIX : <http://lv2plug.in/ns/lv2core#>\n"
				   "PREFIX ll: <http://ll-plugins.nongnu.org/lv2/namespace#>\n"
				   "SELECT ?index, ?hint WHERE { \n"
				   "<>        :port       ?port. \n"
				   "?port     :index      ?index. \n"
				   "?port     :portHint   ?hint. }",
				   plug_iter->first));
      if (!results)
	return -1;
      
      while (!librdf_query_results_finished(results.get())) {
	librdf_node* n = librdf_query_results_get_binding_value(results.get(), 0);
	int port_index = atof((char*)librdf_node_get_literal_value(n));
	string hint = (char*)librdf_uri_as_string(librdf_node_get_uri(librdf_query_results_get_binding_value(results.get(), 1)));
	if (hint == "<http://lv2plug.in/ns#toggled")
	  ports[port_index].toggled = true;
	if (hint == "<http://lv2plug.in/ns#integer")
	  ports[port_index].integer = true;
	if (hint == "<http://lv2plug.in/ns#logarithmic")
	  ports[port_index].logarithmic = true;
	librdf_query_results_next(results.get());
      }
    }
    
    info[plug_iter->first] = ports;
  }
  
  // write the header file
  string header_guard = argv[2];
  for (size_t i = 0; i < header_guard.size(); ++i) {
    char& c = header_guard[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')))
      header_guard[i] = '_';
  }
  ofstream fout(argv[2]);
  fout<<"#ifndef "<<header_guard<<endl
      <<"#define "<<header_guard<<endl<<endl<<endl;
  
  // define data structure
  fout<<"#ifndef PEG_STRUCT"<<endl
      <<"#define PEG_STRUCT"<<endl
      <<"typedef struct {"<<endl
      <<"  float min;"<<endl
      <<"  float max;"<<endl
      <<"  float default_value;"<<endl
      <<"  char toggled;"<<endl
      <<"  char integer;"<<endl
      <<"  char logarithmic;"<<endl
      <<"} peg_data_t;"<<endl
      <<"#endif"<<endl<<endl;
    
  map<string, map<int, PortInfo> >::const_iterator piter;
  for (piter = info.begin(); piter != info.end(); ++piter) {
    fout<<"/* "<<piter->first<<" */"<<endl<<endl;
    
    // write the URI
    fout<<"static const char "<<plugins[piter->first]<<"_uri[] = \""
        <<piter->first<<"\";"<<endl<<endl;
    
    // write port labels
    fout<<"enum "<<plugins[piter->first]<<"_port_enum {"<<endl;
    map<int, PortInfo>::const_iterator iter;
    for (iter = piter->second.begin(); iter != piter->second.end(); ++iter)
      fout<<"  "<<plugins[piter->first]<<"_"<<iter->second.name<<","<<endl;
    fout<<"  "<<plugins[piter->first]<<"_n_ports"<<endl
        <<"};"<<endl<<endl;
    
    // write port info
    fout<<"static const peg_data_t "
        <<plugins[piter->first]<<"_ports[] = {"<<endl;
    for (iter = piter->second.begin(); iter != piter->second.end(); ++iter) {
      fout<<"  { "
          <<iter->second.min<<", "
          <<iter->second.max<<", "
          <<iter->second.default_value<<", "
          <<(iter->second.toggled ? "1" : "0")<<", "
          <<(iter->second.integer ? "1" : "0")<<", "
          <<(iter->second.logarithmic ? "1" : "0")<<" }, "<<endl;
    }
    fout<<"};"<<endl<<endl<<endl;
    
  }
  
  fout<<"#endif /* "<<header_guard<<" */"<<endl;
  
  return 0;
}
