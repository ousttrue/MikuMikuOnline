//
// FieldPlayer.hpp
//

#pragma once
#include "Character.hpp"
#include "CharacterManager.hpp"
#include "CharacterDataProvider.hpp"
#include "MotionPlayer.hpp"
#include "Timer.hpp"
#include "../InputManager.hpp"
#include "../ResourceManager.hpp"

class Stage;
typedef std::shared_ptr<Stage> StagePtr;

struct PlayerStatus
{
    PlayerStatus(const VECTOR& pos_ = VGet(0, 0, 0), const VECTOR& vel_ = VGet(0, 0, 0), const VECTOR& acc_ = VGet(0, 0, 0),
        float roty_ = 0, float roty_speed_ = 0, int motion_ = 0, int prev_motion_ = 0,
        float blend_ratio_ = 0, bool is_walking_ = false) :
    pos(pos_), vel(vel_), acc(acc_), roty(roty_),
        roty_speed(roty_speed_), motion(motion_), prev_motion(prev_motion_),
        blend_ratio(blend_ratio_), is_walking(is_walking_)
    {}

    VECTOR pos, vel, acc; // プレイヤーのマップ上での位置、速度、加速度
    float roty, roty_speed; // プレイヤーの鉛直軸周りの回転角
    int motion, prev_motion; // モーションの種類(BasicMotion::hoge)
    float blend_ratio; // モーションのブレンド比
    bool is_walking; // true:歩き, false:走り
};

// フィールド上のキャラクタ
class FieldPlayer : public Character
{
public:
    FieldPlayer(CharacterDataProvider& data_provider, const StagePtr& stage, const TimerPtr& timer);

    void Draw() const;
    void Update();
    void Init(tstring model_path);
    void ResetPosition();
    void RescuePosition();

	void LoadModel(const tstring& name);
    void SetModel(const ModelHandle& model);
	void PlayMotion(const tstring& name);

public:
    const ModelHandle& model_handle() const;
    const PlayerStatus& current_stat() const;
    float model_height() const;
    bool any_move() const;

    void LinkToCamera(float* roty);
    void UnlinkToCamera();
    void UpdateInput(InputManager* input);

private:
    void Move();
    void InputFromUser();
	void Chara_ShadowRender() const;

private:
    PlayerStatus prev_stat_, current_stat_;
    float model_height_;
	float flight_duration_ideal_;
	float jump_height_;
	int prev_mouse_pos_y_;
    std::unique_ptr<MotionPlayer> motion_player_;
	std::pair<bool,int> additional_motion_;
    TimerPtr timer_;

private:
    ModelHandle model_handle_;
    ModelHandle loading_model_handle_;
    StagePtr stage_;
    bool any_move_;
    InputManager input_;
	int shadow_handle_;

    struct {
        int stand_, walk_, run_;
    } motion;

	int dummy_move_count_;

    CharacterDataProvider& data_provider_;

    float* camera_roty_;
};
typedef std::shared_ptr<FieldPlayer> FieldPlayerPtr;
