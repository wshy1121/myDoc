//
//  "$Id: SignalTemplate.h 45545 2011-12-20 09:30:03Z wang_haifeng $"
//
//  Copyright (c)1992-2007, ZheJiang Dahua Technology Stock CO.LTD.
//  All Rights Reserved.
//
//	Description:	
//	Revisions:		Year-Month-Day  SVN-Author  Modification
//

/// \class SIGNAL_SIGNAL
/// \brief �ź���ģ�顣
///
/// һ���źű�����ʱ��������������ӵ����ϵ�TFunctionN����ֻ֧��
/// ����ֵΪvoid��TFunctionN��SIGNAL_SIGNAL��һ���꣬���ݲ��������ᱻ�滻��
/// TSignalN���û�ͨ�� TSignalN<T1, T2, T3,..,TN>��ʽ��ʹ�ã�TN��ʾ�������ͣ� 
/// N��ʾ��������������Ŀǰ������Ϊ6��
/// \see FUNCTION_FUNCTION

#define SIGNAL_SIGNAL MACRO_JOIN(TSignal,SIGNAL_NUMBER)
#define FUNCTION_FUNCTION MACRO_JOIN(TFunction, SIGNAL_NUMBER)


#include "Base/Object.h"
#if (SIGNAL_NUMBER != 0)
template <SIGNAL_CLASS_TYPES>
#endif

class SIGNAL_SIGNAL
{

	/// �źŽڵ�״̬
	enum SlotState
	{
		slotStateEmpty,		///< �ڵ�Ϊ��
		slotStateNormal,	///< �ڵ�������
	};

public:
	typedef void (CObject::*SigProc)(SIGNAL_TYPE_ARGS);

	/// �ڵ���ص�λ��
	enum SlotPosition
	{
		any,				///< ����ط�
		back,				///< ������ĩβ
		front				///< ������ͷ��
	};

	/// �źŲ���������
	enum ErrorCode
	{
		errorNoFound = -1,		///< û���ҵ��ƶ��Ķ���
		errorExsist = -2,		///< �����Ѿ�����
		errorFull = -3,			///< �Ѿ������ܹ����ӵĶ����������
		errorEmptyProc = -4,	///< ��������ĺ���ָ��Ϊ�գ�û������
	};

	/// ���ź�ģ���������ƥ��ĺ���ָ���������
	typedef FUNCTION_FUNCTION<void SIGNAL_TYPES_COMMA> Proc;

private:
	/// �źŽڵ�ṹ
	struct SignalSlot
	{
		Proc proc;
		SlotState state;
		bool running;
		uint cost;
	};

	int m_numberMax;
	int m_number;
	SignalSlot* m_slots;
	CMutex m_mutex;
	int	m_threadId;	

public:
	/// ���캯��
	/// \param maxSlots �ܹ����ӵ������ָ�����ĸ���
	SIGNAL_SIGNAL(int maxSlots) :
	  m_numberMax(maxSlots), m_number(0)
	{
		m_slots = new SignalSlot[maxSlots];
		for(int i = 0; i < m_numberMax; i++)
		{
			m_slots[i].state = slotStateEmpty;
			m_slots[i].running = false;
		}
	}
	
	 /// ��������
	~SIGNAL_SIGNAL()
	{
		delete []m_slots;
	}

	/// ���ܳ�Ա����ָ�룬�����Ա�����������󶨲����ء�
	/// \param [in] fun ��ĳ�Ա����
	/// \param [in] o ������ָ��
	int Attach(  CObject * pObj, void(CObject::*fun)(SIGNAL_TYPE_ARGS), SlotPosition position = any)
	{
		Proc proc(fun,pObj);
		return Attach(proc, position);
	}

	/// ���غ���ָ�����
	/// \param proc ����ָ�����
	/// \param position �������λ��
	/// \retval >=0 ���ú��Ѿ����ص��źŵĺ���ָ��������
	/// \retval <0 errorCode���͵Ĵ�����
	int Attach(Proc proc, SlotPosition position = any)
	{
		int i;
		
		if(proc.empty())
		{
			return errorEmptyProc;
		}

		if(IsAttached(proc))
		{
			return errorExsist;
		}

		CGuard guard(m_mutex);
		
		switch(position)
		{
		case any:
			for(i = 0; i < m_numberMax; i++)
			{
				if(m_slots[i].state == slotStateEmpty)
				{
					m_slots[i].proc  = proc;
					m_slots[i].state = slotStateNormal;
					m_number++;

					return m_number;
				}
			}
			break;
		case back:
			for(i = m_numberMax - 1; i >= 0; i--)
			{
				if(m_slots[i].state == slotStateEmpty)
				{
					for(int j = i; j < m_numberMax - 1; j++)
					{
						m_slots[j] = m_slots[j + 1];
					}
					m_slots[m_numberMax - 1].proc  = proc;
					m_slots[m_numberMax - 1].state = slotStateNormal;
					m_number++;

					return m_number;
				}
			}
			break;
		case front:
			for(i = 0; i < m_numberMax; i++)
			{
				if(m_slots[i].state == slotStateEmpty)
				{
					for(int j = i; j > 0; j--)
					{
						m_slots[j] = m_slots[j - 1];
					}
					m_slots[0].proc  = proc;
					m_slots[0].state = slotStateNormal;
					m_number++;

					return m_number;
				}
			}
			break;
		}

		return errorFull;
	}			

	/// ���ܳ�Ա����ָ�룬�����Ա�����������󶨡�
	/// ж�غ���ָ����󣬸��ݶ����б���ĺ���ָ����ƥ�䡣
	/// \param [in] fun ��ĳ�Ա����
	/// \param [in] o ������ָ��
	int Detach( CObject * pObj, void(CObject::*fun)(SIGNAL_TYPE_ARGS), bool wait = false)
	{
		Proc proc(fun,pObj);
		return Detach(proc, wait);
	}

	/// ж�غ���ָ����󣬸��ݶ����б���ĺ���ָ����ƥ�䡣
	/// \param proc ����ָ�����
	/// \param wait �Ƿ�ȴ����ڽ��еĻص�������һ����ʹ���߶���������ʱ����Ҫ�ȴ���
	///             ������ڻص�������ж�أ����ܵȴ����ȴ�Ҫ�ر�С�ģ���ֹ����.
	/// \retval >=0 ���ú��Ѿ����ص��źŵĺ���ָ��������
	/// \retval <0 errorCode���͵Ĵ�����
	int Detach(Proc proc, bool wait = false)
	{
		int i;

		if(proc.empty())
		{
			return errorEmptyProc;
		}

		CGuard guard(m_mutex);

		for(i = 0; i < m_numberMax; i++)
		{
			if(m_slots[i].proc ==  proc
				&& m_slots[i].state == slotStateNormal)
			{
				m_slots[i].state = slotStateEmpty;
				m_number--;
				
				/// �ص��̺߳�stop�̲߳���ͬһ�߳�ʱ������Ҫ�ȴ�������ȴ�����������
				if(wait && m_slots[i].running && ThreadGetID() != m_threadId)
				{
					int count = 0;
					while(m_slots[i].running)
					{
						m_mutex.Leave();
						CTime::sleep(1);
						if (++count > 3000)
						{
							count = 0;
							infof("SIGNAL_SIGNAL::Detach wait callback exit! m_threadId=%d\n", m_threadId);
						}
						m_mutex.Enter();
			
					}
				}

				return m_number;
			}
		};
		
		return errorNoFound;
	}

	/// ���ܳ�Ա����ָ�룬�����Ա�����������󶨡�
	/// �жϺ����Ƿ����
	/// \param [in] fun ��ĳ�Ա����
	/// \param [in] o ������ָ��
	bool IsAttached( CObject * pObj, void(CObject::*fun)(SIGNAL_TYPE_ARGS))
	{
		Proc proc(fun,pObj);
		return IsAttached(proc);
	}

	/// �ж�ж�غ���ָ������Ƿ���أ����ݶ����б���ĺ���ָ����ƥ�䡣
	/// \param proc ����ָ�����
	bool IsAttached(Proc proc)
	{
		CGuard guard(m_mutex);

		if(proc.empty())
		{
			return false;
		}

		for(int i = 0; i < m_numberMax; i++)
		{
			if(m_slots[i].proc ==  proc
				&& m_slots[i].state == slotStateNormal)
			{
				return true;
			}
		}

		return false;
	}

	/// ����()������������Ժ����������ʽ���������ӵ��źŵ����к���ָ�롣
	inline void operator()(SIGNAL_TYPE_ARGS)
	{
		int i;

		CGuard guard(m_mutex);
		uint64 us1 = 0, us2 = 0;
		
		m_threadId = ThreadGetID(); // ����ص��߳�ID
		
		for(i = 0; i < m_numberMax; i++) // call back functions one by one
		{
			if(m_slots[i].state == slotStateNormal)
			{
				Proc temp = m_slots[i].proc;
				
				m_slots[i].running = true;
				m_mutex.Leave();
				
				// ����ִ��������ͳ��
	 			us1 = CTime::getCurrentMicroSecond();
				temp(SIGNAL_ARGS);
				us2 = CTime::getCurrentMicroSecond();
				m_slots[i].cost = (us1 <= us2) ? uint(us2 - us1) : 1;
				
				m_mutex.Enter();
				m_slots[i].running = false;
			}
		}
	}
	
	void stat()
	{
		int i;

		CGuard guard(m_mutex);
		
		for(i = 0; i < m_numberMax; i++)
		{
			if(m_slots[i].state == slotStateNormal)
			{
				printf("\t%8d us, %p, %s\n",
				 m_slots[i].cost,
				 m_slots[i].proc.getObject(),
				 m_slots[i].proc.getObjectType());
			}
		}			
	}
};

#undef SIGNAL_SIGNAL
#undef FUNCTION_FUNCTION

