#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include "utils.h"
#include "keyboard4x3_manager.h"

#include "Timer.h"
#include "FiniteStateMachine.h"


#define UI_BEDUG (true)


struct UI
{
protected:
	// Events:
	FSM_CREATE_EVENT(success_e,   0);
	FSM_CREATE_EVENT(granted_e,   0);
	FSM_CREATE_EVENT(denied_e,    0);
	FSM_CREATE_EVENT(timeout_e,   0);
	FSM_CREATE_EVENT(input_e,     0);
	FSM_CREATE_EVENT(cancel_e,    0);
	FSM_CREATE_EVENT(enter_e,     0);
	FSM_CREATE_EVENT(start_e,     0);
	FSM_CREATE_EVENT(stop_e,      0);
	FSM_CREATE_EVENT(limit_min_e, 0);
	FSM_CREATE_EVENT(limit_max_e, 0);
	FSM_CREATE_EVENT(end_e,       0);
	FSM_CREATE_EVENT(solved_e,    0);
	FSM_CREATE_EVENT(load_e,      1);
	FSM_CREATE_EVENT(reboot_e,    2);
	FSM_CREATE_EVENT(error_e,     3);

private:
	// States:
	struct _init_s       { void operator()(); };
	struct _load_s       { void operator()(); };
	struct _idle_s       { void operator()(); static constexpr uint32_t TIMEOUT_MS = 10000 ;};
	struct _denied_s     { void operator()(); };
	struct _granted_s    { void operator()(); };
	struct _limit_s      { void operator()(); static utl::Timer blinkTimer; static bool limitPage; };
	struct _input_s      { void operator()(); };
	struct _check_s      { void operator()(); };
	struct _wait_count_s { void operator()(); static constexpr uint32_t TIMEOUT_MS = 180000; };
	struct _count_s      { void operator()(); };
	struct _record_s     { void operator()(); };
	struct _save_s       { void operator()(); static constexpr uint32_t TIMEOUT_MS = 30000; static bool loading;};
	struct _result_s     { void operator()(); static constexpr uint32_t TIMEOUT_MS = 120000; };
	struct _reboot_s     { void operator()(); static constexpr uint32_t TIMEOUT_MS = 2500; };
	struct _error_s      { void operator()(); };

	FSM_CREATE_STATE(init_s,       _init_s);
	FSM_CREATE_STATE(load_s,       _load_s);
	FSM_CREATE_STATE(idle_s,       _idle_s);
	FSM_CREATE_STATE(denied_s,     _denied_s);
	FSM_CREATE_STATE(granted_s,    _granted_s);
	FSM_CREATE_STATE(limit_s,      _limit_s);
	FSM_CREATE_STATE(input_s,      _input_s);
	FSM_CREATE_STATE(check_s,      _check_s);
	FSM_CREATE_STATE(wait_count_s, _wait_count_s);
	FSM_CREATE_STATE(count_s,      _count_s);
	FSM_CREATE_STATE(record_s,     _record_s);
	FSM_CREATE_STATE(save_s,       _save_s);
	FSM_CREATE_STATE(result_s,     _result_s);
	FSM_CREATE_STATE(reboot_s,     _reboot_s);
	FSM_CREATE_STATE(error_s,      _error_s);

	// Actions:
	struct load_ui_a        { void operator()(); };
	struct reboot_ui_a      { void operator()(); };
	struct error_ui_a       { void operator()(); };
	struct idle_ui_a        { void operator()(); };
	struct granted_ui_a     { void operator()(); };
	struct denied_ui_a      { void operator()(); };
	struct limit_ui_a       { void operator()(); };
	struct reset_input_ui_a { void operator()(); };
	struct check_a          { void operator()(); };
	struct start_a          { void operator()(); };
	struct wait_count_ui_a  { void operator()(); };
	struct count_a          { void operator()(); };
	struct record_ui_a      { void operator()(); };
	struct start_save_a     { void operator()(); };
	struct show_load_a      { void operator()(); };
	struct result_ui_a      { void operator()(); };

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s,       success_e,   load_s,       load_ui_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<load_s,       error_e,     error_s,      error_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<load_s,       success_e,   idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s,       error_e,     error_s,      error_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,       reboot_e,    reboot_s,     reboot_ui_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,       load_e,      load_s,       load_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,       granted_e,   granted_s,    granted_ui_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,       denied_e,    denied_s,     denied_ui_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<granted_s,    timeout_e,   limit_s,      limit_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<denied_s,     timeout_e,   idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<limit_s,      input_e,     input_s,      reset_input_ui_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<limit_s,      timeout_e,   idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<limit_s,      cancel_e,    idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<limit_s,      start_e,     check_s,      start_a,          fsm::Guard::NO_GUARD>,

		fsm::Transition<input_s,      cancel_e,    idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<input_s,      timeout_e,   idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<input_s,      enter_e,     check_s,      check_a,          fsm::Guard::NO_GUARD>,
		fsm::Transition<input_s,      start_e,     check_s,      start_a,          fsm::Guard::NO_GUARD>,

		fsm::Transition<check_s,      limit_min_e, input_s,      reset_input_ui_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<check_s,      limit_max_e, limit_s,      limit_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<check_s,      success_e,   wait_count_s, wait_count_ui_a,  fsm::Guard::NO_GUARD>,

		fsm::Transition<wait_count_s, end_e,       record_s,     record_ui_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<wait_count_s, enter_e,     count_s,      count_a,          fsm::Guard::NO_GUARD>,

		fsm::Transition<count_s,      end_e,       wait_count_s, wait_count_ui_a,  fsm::Guard::NO_GUARD>,

		fsm::Transition<record_s,     error_e,     error_s,      error_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<record_s,     success_e,   save_s,       start_save_a,     fsm::Guard::NO_GUARD>,

		fsm::Transition<save_s,       timeout_e,   error_s,      error_ui_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,       granted_e,   save_s,       show_load_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,       denied_e,    save_s,       show_load_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,       success_e,   result_s,     result_ui_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<result_s,     end_e,       load_s,       load_ui_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<result_s,     granted_e,   granted_s,    granted_ui_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<result_s,     denied_e,    denied_s,     denied_ui_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<reboot_s,     timeout_e,   idle_s,       idle_ui_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<error_s,      solved_e,    load_s,       load_ui_a,        fsm::Guard::NO_GUARD>
	>;

protected:
	static constexpr uint32_t BASE_TIMEOUT_MS = 30000;
	static constexpr uint32_t BLINK_DELAY_MS = 1000;
	static constexpr char TAG[] = "UI";

	static fsm::FiniteStateMachine<fsm_table> fsm;
	static utl::Timer timer;
	static uint32_t card;
	static uint8_t limitBuffer[KEYBOARD4X3_BUFFER_SIZE];
	static uint8_t currentBuffer[KEYBOARD4X3_BUFFER_SIZE];
	static uint8_t resultBuffer[KEYBOARD4X3_BUFFER_SIZE];
	static uint32_t targetMl;
	static uint32_t resultMl;

	static bool isEnter();
	static bool isCancel();
	static bool isStart();
	static bool isStop();

	static void setCard(uint32_t card);

public:
	static void proccess();

	static void setReboot();

	static void resetResultMl();
	static uint32_t getResultMl();

	static uint32_t getCard();

	static bool isPumpWorking();

};


#endif
