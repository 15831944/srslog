#include "json_process.h"
#include <stdio.h>
#include <string.h>

void parse_json_no_array(const char* my_json_string,
                         const std::vector<std::string> &keys,
                         std::map<std::string, std::string> &res)
{
     cJSON *root = cJSON_Parse(my_json_string);
     if (NULL == root) {
         return;
     }

     //begin parse
     for (std::vector<std::string>::const_iterator iter = keys.begin();
          iter != keys.end();
          ++iter) {

         cJSON *key = cJSON_GetObjectItem(root,(*iter).c_str());
         if (NULL != key) {
            res.insert(std::make_pair(*iter, key->valuestring));
         }
     }


     //end parse
     cJSON_Delete(root);
}


void get_value_from_map(const std::map<std::string, std::string> &data_map, const std::string &key, std::string &res)
{
    for (std::map<std::string, std::string>::const_iterator iter = data_map.begin();
         iter != data_map.end();
         ++iter) {
        if (iter->first == key) {
            res = iter->second;
            break;
        }
    }
}
