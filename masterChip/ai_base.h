#ifndef AI_BASE_H
#define AI_BASE_H

#include <vector>
#include <string>
#include <fstream>

#include <nncase/runtime/interpreter.h>
#include "scoped_timing.hpp"

using std::string;
using std::vector;
using namespace nncase::runtime;

/**
 * @brief AI基类，封装nncas
 */
class AIBase
{
public:
    /**
     * @brief AI基类构造函数，加载kmodel,并初始化kmodel输入、输出
     * @param kmodel_file kmodel文件路径
     * @param debug_mode
     * @return None
     */
    AIBase(const char *kmodel_file,const string model_name, const int debug_mode = 1);

    /**
     * @brief AI基类析构函数
     * @return None
     */
    ~AIBase();

    /**
     * @brief 根据索引获取kmodel输入tensor
     * @param idx 输入数据指针
     * @return None
     */
    runtime_tensor get_input_tensor(size_t idx);

    /**
     * @brief 推理kmodel
     * @return None
     */
    void run();

    /**
     * @brief 获取kmodel输出，结果保存在对应的类属性中
     * @return None
     */
    void get_output();

protected:
    string model_name_;                   
    int debug_mode_;                       
    vector<float *> p_outputs_;            
    vector<vector<int>> input_shapes_;     
    vector<vector<int>> output_shapes_;   
    vector<int> each_input_size_by_byte_;  
    vector<int> each_output_size_by_byte_; 
private:
    /**
     * @brief 首次初始化kmodel输入，并获取输入shape
     * @return None
     */
    void set_input_init();

    /**
     * @brief 首次初始化kmodel输出，并获取输出shape
     * @return None
     */
    void set_output_init();

    interpreter kmodel_interp_;
    vector<unsigned char> kmodel_vec_; 
};
#endif