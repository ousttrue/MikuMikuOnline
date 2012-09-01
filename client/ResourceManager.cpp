//
// ResourceManager.cpp
//

#include <fstream>
#include <boost/filesystem.hpp>
#include "ResourceManager.hpp"
#include "../common/Logger.hpp"
#include "../common/unicode.hpp"

const static TCHAR* CHAT_FONT_NAME = _T("UmePlus P Gothic");
static int CHAT_FONT_SIZE = 15;
static int CHAT_FONT_THICK = 1;
static int CHAT_FONT_TYPE = DX_FONTTYPE_ANTIALIASING;

int ResourceManager::default_font_handle_ = -1;
int ResourceManager::default_font_handle()
{
    if (default_font_handle_ < 0) {
        //TCHAR font_name[] = CHAT_FONT_NAME;
        const TCHAR* font_name = CHAT_FONT_NAME;
        default_font_handle_ = CreateFontToHandle(font_name, CHAT_FONT_SIZE, CHAT_FONT_THICK, CHAT_FONT_TYPE);
    }

    return default_font_handle_;

}

int ResourceManager::default_font_size()
{
    return CHAT_FONT_SIZE;
}

std::unordered_map<tstring, ImageHandlePtr> ResourceManager::graph_handles_;
std::unordered_map<tstring, std::vector<ImageHandlePtr>> ResourceManager::div_graph_handles_;
ImageHandlePtr ResourceManager::LoadCachedGraph(const tstring& filename)
{
    ImageHandlePtr handle;
    if(graph_handles_.find(filename) == graph_handles_.end()) {
        handle = std::make_shared<ImageHandle>(DxLib::LoadGraph(filename.c_str()));
        graph_handles_[filename] = handle;
    } else {
        handle = graph_handles_[filename];
    }
    return ImageHandlePtr(handle);
}

void ResourceManager::ClearCache()
{
    graph_handles_.clear();
    div_graph_handles_.clear();

    model_names_.clear();
    model_handles_.clear();
    model_name_tree_.clear();

    InitGraph();
    MV1InitModel();
}

std::unordered_map<tstring, tstring> ResourceManager::model_names_;
std::unordered_map<tstring, ModelHandle> ResourceManager::model_handles_;
ptree ResourceManager::model_name_tree_;
void ResourceManager::BuildModelFileTree()
{
    using namespace boost::filesystem;
    using namespace std;

    model_name_tree_.clear();

    path p("./resources/models");

    try {
        if (exists(p) && is_directory(p)) {
            for (auto it_dir = directory_iterator(p); it_dir != directory_iterator(); ++it_dir) {
                if (is_directory(*it_dir)) {
                    path json_path = it_dir->path() / "info.json";
                    if (exists(json_path)) {

                        path model_path;
                        for (auto it = directory_iterator(*it_dir); it != directory_iterator(); ++it) {
                            auto extension = it->path().extension().string();

                            if (extension == ".mv1" || extension == ".x"
                             || extension == ".pmd" || extension == ".pmx") {
                                model_path = it->path();
                                break;
                            }
                        }

                        if (!model_path.empty()) {
                            ptree pt_json;
                            read_json(json_path.string(), pt_json);

                            std::string name = pt_json.get<std::string>("name", "");
                            pt_json.put<std::string>("modelpath", unicode::sjis2utf8(model_path.string()));

                            if (name.size() > 0) {
                                model_name_tree_.put_child(ptree::path_type(name + ":_info_", ':'), pt_json);
                            }
                        }
                    }
                }
            }
        }
    } catch (const filesystem_error& ex) {
        Logger::Error(_T("%s"), unicode::ToTString(ex.what()));
    }

}

struct ReadFuncData {
        boost::filesystem::wpath model_dir;
        std::list<std::pair<std::string, std::string>> motions;
        std::list<std::pair<std::string, std::string>>::iterator motions_it;
};

int LoadFile(const TCHAR *FilePath, void **FileImageAddr, int *FileSize)
{
    Logger::Debug(_T("Load %s"), FilePath);
    tstring path(FilePath);
    std::ifstream ifs(path.c_str(), std::ios::binary);

    if (!ifs) {
        *FileImageAddr = nullptr;
        return -1;
    }

    ifs.seekg (0, std::ios::end);
    *FileSize = static_cast<int>(ifs.tellg());
    ifs.seekg (0, std::ios::beg);

    auto buffer = new char[*FileSize];
    ifs.read(buffer, *FileSize);
    *FileImageAddr = buffer;

    return 0;
}

int FileReadFunc(const TCHAR *FilePath, void **FileImageAddr, int *FileSize, void *FileReadFuncData)
{
    ReadFuncData& funcdata = *static_cast<ReadFuncData*>(FileReadFuncData);

    using namespace boost::filesystem;
    wpath filepath(FilePath);

    bool load_motion = false;
    if (funcdata.motions_it != funcdata.motions.end() &&
            filepath.string().find_last_of("L.vmd") != std::string::npos) {

        filepath = funcdata.motions_it->second;
        load_motion = true;
    }

    Logger::Debug(_T("Request %s"), unicode::ToTString(filepath.wstring()));

    wpath full_path = funcdata.model_dir / filepath;
    if (!exists(full_path)) {
        if (load_motion) {
            full_path = "./resources/motions" / filepath;
        }
    }

    int result = LoadFile(full_path.wstring().c_str(), FileImageAddr, FileSize);

    if (load_motion) {
        // 読み込み失敗したモーションを削除
        if (result == -1) {
            funcdata.motions_it->second = "";
        }
        ++funcdata.motions_it;
    }

    return result;
}

int FileReleaseFunc(void *MemoryAddr, void *FileReadFuncData)
{
    delete static_cast<char*>(MemoryAddr);
    return 0;
}

ModelHandle ResourceManager::LoadModelFromName(const tstring& name)
{
    if (model_name_tree_.empty()) {
        BuildModelFileTree();
    }

    std::string filepath;
    ptree info;

    Logger::Debug(_T("NAME: %s"), unicode::ToTString(unicode::ToString(name)));

    auto name_it = model_names_.find(name);
    if (name_it != model_names_.end()) {
        filepath = unicode::ToString(name_it->second);

    } else {
        ptree p;
        auto path = ptree::path_type(unicode::ToString(name), ':');

        p = model_name_tree_.get_child(path, ptree());

        // ルートで探索を打ち切る
        while (!path.single()) {
            if (p.empty()) {
                Logger::Debug(_T("EMPTY %s"), unicode::ToTString(path.dump()));
                // 親ノードを検索
                std::string path_str = path.dump();
                size_t separator_pos = path_str.find_last_of(':');
                assert(separator_pos != std::string::npos);

                path = ptree::path_type(path_str.substr(0, separator_pos), ':');
                p = model_name_tree_.get_child(path, ptree());
            } else {
                info = p.get_child("_info_", ptree());
                if (info.empty()) {
                    Logger::Debug(_T("CHILD_FOUND"));
                    // データがない場合は最初の子ノードへ移動
                    p = p.get_child(ptree::path_type(p.front().first, ':'), ptree());
                } else {
                    Logger::Debug(_T("FOUND"));
                    break;
                }
            }

        }

        filepath = info.get<std::string>("modelpath", "");
        Logger::Debug(_T("ModelName to filepath %s -> %s"), name, unicode::ToTString(filepath));
        model_names_[name] = unicode::ToTString(filepath);
    }

    if (filepath.size() > 0) {
        auto it = model_handles_.find(unicode::ToTString(filepath));
        if (it != model_handles_.end()) {
            return it->second.Clone();
        } else {
            MV1SetLoadModelPhysicsWorldGravity(-100);
            MV1SetLoadModelUsePhysicsMode(DX_LOADMODEL_PHYSICS_LOADCALC);

            ReadFuncData funcdata;
            // funcdata.model_dir = boost::filesystem::path(unicode::utf82sjis(filepath)).parent_path();
            funcdata.model_dir = boost::filesystem::wpath(unicode::ToWString(filepath)).parent_path();

            auto motions = info.get_child("character.motions", ptree());
            for (auto it = motions.begin(); it != motions.end(); ++it) {
                funcdata.motions.push_back(
                        std::pair<std::string, std::string>(it->first,
                                        it->second.get_value<std::string>()));
            }
            funcdata.motions_it = funcdata.motions.begin();

            void *FileImage ;
            int FileSize ;

            LoadFile(unicode::ToTString(filepath).c_str(), &FileImage, &FileSize );
            int handle = MV1LoadModelFromMem( FileImage, FileSize, FileReadFunc, FileReleaseFunc, &funcdata);

            // モーションの名前を設定
            int motion_index = 0;
            for (auto it = motions.begin(); it != motions.end(); ++it) {
                MV1SetAnimName(handle, motion_index, unicode::ToTString(it->first).c_str());
                Logger::Debug(_T("Motion  %d"), unicode::ToTString(it->first));
                motion_index++;
            }

            auto model_handle = ModelHandle(handle, std::make_shared<ptree>(info));
            model_handles_[unicode::ToTString(filepath)] = model_handle;

            Logger::Debug(_T("Model  %d"), handle);
            return model_handle.Clone();
        }
    } else {
        return ModelHandle();
    }
}

void ResourceManager::CacheBakedModel()
{

}

ImageHandle::ImageHandle() :
                handle_(-1)
{

}

ImageHandle::ImageHandle(int handle) :
        handle_(handle)
{
}

ImageHandle::operator int() const
{
    return handle_;
}

ModelHandle::ModelHandle(int handle, const std::shared_ptr<ptree>& property, bool async_load) :
        handle_(handle),
        property_(property),
        name_(property_->get<std::string>("name", "")),
        async_load_(async_load)
{

}

ModelHandle ModelHandle::Clone()
{
    if (CheckHandleASyncLoad(handle_) == TRUE) {
        return ModelHandle(MV1DuplicateModel(handle_), property_, true);
    } else {
        return ModelHandle(MV1DuplicateModel(handle_), property_);
    }
}

ModelHandle::ModelHandle() :
        handle_(-1),
        property_(std::make_shared<ptree>())
{

}

ModelHandle::~ModelHandle()
{

}

int ModelHandle::handle() const
{
    return handle_;
}

const ptree& ModelHandle::property() const
{
    return *property_;
}

std::string ModelHandle::name() const
{
    return name_;
}
