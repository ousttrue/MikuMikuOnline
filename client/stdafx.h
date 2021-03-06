// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#ifdef _WIN32

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows ヘッダー ファイル:
#include <windows.h>
#include <DxLib.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#endif

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdint.h>
#include <assert.h>
#include <cmath>
#include <iostream>

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。

#include <vector>
#include <list>
#include <map>
#include <unordered_set>
#include <deque>
#include <queue>
#include <array>
#include <string>
#include <memory>
#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <pssr.h>
#include <osrng.h>
#include <modes.h>
#include <aes.h>
#include <rsa.h>
#include <sha.h>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/property_tree/ptree_serialization.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/string.hpp>

#include "../common/Logger.hpp"
#include "Language.hpp"

#include "tlsf.h"