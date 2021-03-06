#pragma once

#include <vector>
#include <array>
#include <string>
#include <DxLib.h>
#include "../../common/unicode.hpp"
#include "../InputManager.hpp"
#include "Character.hpp"
#include "CharacterManager.hpp"
#include "CharacterDataProvider.hpp"
#include "FieldPlayer.hpp"
#include "../ResourceManager.hpp"
#include "../ManagerAccessor.hpp"
#include "MotionPlayer.hpp"
#include "Timer.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

class Stage;
typedef std::shared_ptr<Stage> StagePtr;

struct PointF {
    PointF(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
    float x, y;
};

struct CameraStatus
{
	CameraStatus(float radius_ = 0, float target_height_ = 0,
		float theta_ = 0, float phi_ = 0, float manual_control_ = 0,
		const std::pair<int, int>& manual_control_startpos_ = std::pair<int, int>()) : 
	radius(radius_), target_height(target_height_), theta(theta_),
		phi(phi_), manual_control(manual_control_),
		manual_control_startpos(manual_control_startpos) {}

	//float height;           // カメラY座標のプレイヤー身長に対する比率
	float radius;          // プレイヤーを中心とするカメラ回転軌道の半径
	float target_height;    // 注視点Y座標のプレイヤー身長に対する比率
	float theta, phi;    // カメラの横方向の回転thetaと、縦方向の回転phi、単位はラジアン
	bool manual_control; // true:カメラの方向決定は手動モード
	std::pair<int, int> manual_control_startpos; // 手動モードになった瞬間のマウス座標
};

class KeyChecker
{
public:
	int Check();
	int GetKeyCount(size_t key_code) const;

private:
	std::array<int, 256> key_count_;
};

class LightStatus
{
public:
	LightStatus(float latitude, float longitude, const StagePtr& stage) :
		stage_(stage),
		calc_location_(PointF(longitude,latitude)), 
		elevation_(0),
		frame_count_(0),
		noon_flag_(true),
		night_flag_(true),
		init_flag_(true)
		{};
	void Init();
	void MaterialInit();
	void Update();
private:
	void Calc();

	float JuliusYear(int yy,int mm,int dd,int h,int m,int s,int time_differ);
	float SiderealHour(float julius_year,int h,int m,int s,float longitude,int time_differ);
	float SolarPosition1(float julius_year);	// distance to celestical longitude
	float SolarPosition2(float julius_year);	// distance , astronomical unit(AU)
	float SolarPosition3(float julius_year);	// degree of declination
	float SolarPosition4(float julius_year);	// the right ascension degree
	float SolarAltitude(float latitude,float sidereal_hour,float solar_declination,float right_ascension);	// calculating solar altitude
	float SolarDirection(float latitude,float sidereal_hour,float solar_declination,float right_ascension);	// calculating solar direction
	float SolarApparentAltitude1(float altitude,float distance);	// calculating apparent altitude
	float SolarApparentAltitude2(float altitude,float distance);	// calculating apparent altitude (solar distance (AU) )

	void SetGrobalAmbientColorMatchToTime();

	//typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;
	boost::posix_time::ptime time_,moning_glow_,sunset_;
	boost::posix_time::time_duration  time_differ_;
	PointF calc_location_;
	float elevation_;

	std::unordered_map<tstring,int> material_index_of_name_;

	VECTOR light_pos_;
	float light_distance_;
	int frame_count_;

	int light_handle_;



	StagePtr stage_;
	bool init_flag_;

	bool noon_flag_,night_flag_;
};

typedef std::shared_ptr<CharacterManager> CharacterManagerPtr;

class GameLoop
{
public:
	GameLoop(const ManagerAccessorPtr& manager_accessor, const StagePtr& stage);
	int Init(std::shared_ptr<CharacterManager> character_manager);
	int ProcessInput(InputManager* input);
	int Update();
	int Draw();

	FieldPlayerPtr myself() const;
	void ResetCameraPosition();

private:
	// 自分自身
	CharacterManagerPtr charmgr_;
	FieldPlayerPtr myself_;

	ManagerAccessorPtr manager_accessor_;
	StagePtr stage_;
	CameraStatus camera_default_stat, camera;
	LightStatus light;

	void FixCameraPosition();
	void MoveCamera(InputManager* input);

	const static float CAMERA_MIN_RADIUS;
	const static float CAMERA_MAX_RADIUS;

};
