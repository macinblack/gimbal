#include "stm32f1xx_hal.h"
#include "newcontrol.h"

float pidCmd[3];

float pidCmdPrev[3] = {0.0f, 0.0f, 0.0f};

void computeMotorCommands(float dt)
{
  pidCmd[PITCH] = updatePID(pointingCmd[PITCH] * mechanical2electricalDegrees[PITCH],
                                  sensors.margAttitude500Hz[PITCH] * mechanical2electricalDegrees[PITCH],
                                  dt, holdIntegrators, &eepromConfig.PID[PITCH_PID]);
}

float updatePID(float command,/*desired*/
								float state,/*test*/ 
								float deltaT,
								uint8_t iHold, 
								struct PIDdata *PIDparameters)
{
    float error;
    float dTerm;
    float dTermFiltered;
    float dAverage;

	
    error = command - state;

    if (PIDparameters->type == ANGULAR)
        error = standardRadianFormat(error);

    ///////////////////////////////////

    if (iHold == false)
    {
    	PIDparameters->iTerm += error * deltaT;
    	PIDparameters->iTerm = constrain(PIDparameters->iTerm, -PIDparameters->windupGuard, PIDparameters->windupGuard);//?y��??T����
    }

    ///////////////////////////////////
	
    if (PIDparameters->dErrorCalc == D_ERROR)  // Calculate D term from error
    {
			//����1yerror?????�顤???��?1?��?�ꡧde/dt��?��????Dde?����?(error - PIDparameters->lastDcalcValue) 
		dTerm = (error - PIDparameters->lastDcalcValue) / deltaT;
        PIDparameters->lastDcalcValue = error;//�����?�̡�?��??2?
	}
	else                                       // Calculate D term from state
	{
		//��??����?��1��????a��y????��??��D��???����a??3����?����???����??��??DD?�顤?????
		dTerm = (PIDparameters->lastDcalcValue - state) / deltaT;

		if (PIDparameters->type == ANGULAR)//��?1?����D��?a???��
		    dTerm = standardRadianFormat(dTerm);//��a??3��������????��

		PIDparameters->lastDcalcValue = state;//�����?�̡�?����䨬?
	}

    ///////////////////////////////////
		//???�顤?????DD��??���̨�����??2�� deltaT / (rc + deltaT) ?��1??����???2��?�̨�ya ��? rc=1.0f/(2.0f*PI*F)
    dTermFiltered = PIDparameters->lastDterm + deltaT / (rc + deltaT) * (dTerm - PIDparameters->lastDterm);

	//??����������y��??�顤?????DD?��???��
    dAverage = (dTermFiltered + PIDparameters->lastDterm + PIDparameters->lastLastDterm) * 0.333333f;

    PIDparameters->lastLastDterm = PIDparameters->lastDterm;//��?��??�顤???�����?��?��?��?��??�顤???��?����?��?
    PIDparameters->lastDterm = dTermFiltered;//�̡�?��?�顤???�����?��?��?��??�顤???��?����?��?

    ///////////////////////////////////
//����??PID?????��1?
    if (PIDparameters->type == ANGULAR)//��?1?����D��?a???��
        return(PIDparameters->P * error     /*  Kp*e  */           +
	           PIDparameters->I * PIDparameters->iTerm + /*   Ki*?��edt   */
	           PIDparameters->D * dAverage);/*   Kd*�ꡧde/dt��?  */
    else
        return(PIDparameters->P * PIDparameters->B * command /* Kp *(B * point) */  +
               PIDparameters->I * PIDparameters->iTerm /*   Ki*?��edt   */      +
               PIDparameters->D * dAverage   /*   Kd*�ꡧde/dt��?  */ -
               PIDparameters->P * state);//??????��?

    ///////////////////////////////////
}
