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
std::vector<std::string> ResourceManager::model_name_list_;
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
							if (!name.empty()) {
								model_name_list_.push_back(name);
							}

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
        } else {
			full_path = "./resources/textures" / filepath;
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

void SetMotionNames(int handle, const ReadFuncData& funcdata)
{
	// モーションの名前を設定
    int motion_index = 0;
    for (auto it = funcdata.motions.begin(); it != funcdata.motions.end(); ++it) {
		if (!it->second.empty()) {
			MV1SetAnimName(handle, motion_index, unicode::ToTString(it->first).c_str());
			Logger::Debug(_T("Motion  %d"), unicode::ToTString(it->first));
			motion_index++;
		}
    }
}

void ResourceManager::RequestModelFromName(const tstring& name)
{
	if (!IsCachedModelName(name)) {

	}
}

bool ResourceManager::IsCachedModelName(const tstring& name)
{
	auto name_it = model_names_.find(name);
    if (name_it != model_names_.end()) {
		return model_handles_.find(unicode::ToTString(name_it->second)) != model_handles_.end();
	} else {
		return false;
	}
}

ReadFuncData::ReadFuncData(const ptree& info)
{
	tstring filepath = unicode::ToTString(info.get<std::string>("modelpath", ""));
    model_dir = boost::filesystem::wpath(unicode::ToWString(filepath)).parent_path();

    auto motions_array = info.get_child("character.motions", ptree());
    for (auto it = motions_array.begin(); it != motions_array.end(); ++it) {
        motions.push_back(
                std::pair<std::string, std::string>(it->first,
                                it->second.get_value<std::string>()));
    }
    motions_it = motions.begin();
}

float ResourceManager::model_edge_size_ = 1.0f;
ModelHandle ResourceManager::LoadModelFromName(const tstring& name, bool async)
{
 	auto fullpath = ptree::path_type(unicode::ToString(NameToFullPath(name)), ':');
	ptree p = model_name_tree_.get_child(fullpath, ptree());

	ptree info = p.get_child("_info_", ptree());
	tstring filepath = unicode::ToTString(info.get<std::string>("modelpath", ""));
	if(!filepath.size())
	{
		fullpath = ptree::path_type(unicode::ToString(NameToFullPath(_T("char:アンノウン:おぼろ(へうげもん)式"))), ':');
		p = model_name_tree_.get_child(fullpath, ptree());
		info = p.get_child("_info_", ptree());
		filepath = unicode::ToTString(info.get<std::string>("modelpath", ""));
	}
    if (filepath.size() > 0) {
        auto it = model_handles_.find(unicode::ToTString(filepath));
        if (it != model_handles_.end()) {
            return it->second.Clone();
        }else{
			auto funcdata = std::make_shared<ReadFuncData>(info);

            void *FileImage ;
            int FileSize ;

            LoadFile(unicode::ToTString(filepath).c_str(), &FileImage, &FileSize );

			if (async) {
				SetUseASyncLoadFlag(TRUE);
			}

            int handle = MV1LoadModelFromMem( FileImage, FileSize, FileReadFunc, FileReleaseFunc, &(*funcdata));

			if (async) {
				SetUseASyncLoadFlag(FALSE);
			} else {
				SetMotionNames(handle, *funcdata);
			}

            auto model_handle = ModelHandle(handle, funcdata, std::make_shared<ptree>(info), async);
            model_handles_[unicode::ToTString(filepath)] = model_handle;

            Logger::Debug(_T("Model %d"), handle);
            return model_handle.Clone();
		}
    } else {
        return ModelHandle();
    }
}

void ResourceManager::SetModelEdgeSize(int handle)
{
	int MaterialNum = MV1GetMaterialNum( handle );
	for( int i = 0 ; i < MaterialNum ; i ++ )
	{
		// マテリアルの元々の輪郭線の太さを取得
		float dotwidth = MV1GetMaterialOutLineDotWidth( handle, i );
		float width = MV1GetMaterialOutLineWidth( handle , i );
		// マテリアルの輪郭線の太さ
		MV1SetMaterialOutLineDotWidth( handle, i, dotwidth * model_edge_size_);
		MV1SetMaterialOutLineWidth( handle, i, width * model_edge_size_);
	}
}


tstring ResourceManager::NameToFullPath(const tstring& name)
{
    if (model_name_tree_.empty()) {
        BuildModelFileTree();
    }

    tstring fullpath;
    ptree info;

    auto name_it = model_names_.find(name);
    if (name_it != model_names_.end()) {
        fullpath = name_it->second;

    } else {
        ptree p;
        auto path = ptree::path_type(unicode::ToString(name), ':');

        p = model_name_tree_.get_child(path, ptree());

        // ルートで探索を打ち切る
        while (1) {
            if (p.empty()) {
                Logger::Debug(_T("EMPTY %s"), unicode::ToTString(path.dump()));
                // 親ノードを検索
				if (!path.single()) {
					return _T("");
					break;
				}
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
					path /= ptree::path_type(p.front().first, ':');
                    p = p.get_child(ptree::path_type(p.front().first, ':'), ptree());
                } else {
                    Logger::Debug(_T("FOUND"));
                    break;
                }
            }

        }
		
        Logger::Debug(_T("ModelName to fullpath %s -> %s"), name, unicode::ToTString(path.dump()));
		fullpath = unicode::ToTString(path.dump());
		model_names_[name] = fullpath;
    }

	return fullpath;
}

const std::vector<std::string>& ResourceManager::GetModelNameList()
{
	return model_name_list_;
}

void ResourceManager::CacheBakedModel()
{

}

void ResourceManager::set_model_edge_size(float edge_size)
{
	model_edge_size_ = edge_size;
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

ModelHandle::ModelHandle(int handle, const ReadFuncDataPtr& funcdata, const std::shared_ptr<ptree>& property, bool async_load) :
        handle_(handle),
		funcdata_(funcdata),
        property_(property),
        name_(property_->get<std::string>("name", "")),
        async_load_(async_load)
{

}

ModelHandle ModelHandle::Clone()
{
    if (CheckHandleASyncLoad(handle_) == TRUE) {
        return ModelHandle(handle_, funcdata_, property_, true);
    } else {
        return ModelHandle(MV1DuplicateModel(handle_), funcdata_, property_);
    }
}

bool ModelHandle::CheckLoaded()
{
	if (!async_load_) {
		return true;
	} else if (async_load_ && CheckHandleASyncLoad(handle_) == FALSE) {
        handle_ = MV1DuplicateModel(handle_);
		SetMotionNames(handle_, *funcdata_);
		async_load_ = false;
		return true;
	} else {
		return false;
	}
}

ModelHandle::operator bool() const
{
	return handle_ != -1;
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
