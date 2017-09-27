import com.fourinone.Contractor;
import com.fourinone.WareHouse;
import com.fourinone.WorkerLocal;
import java.util.ArrayList;

public class CtorMul extends Contractor
{
	public WareHouse giveTask(WareHouse inhouse)
	{
		WorkerLocal[] wks = getWaitingWorkers("WorkerMul");
		System.out.println("wks.length:"+wks.length);
		
		/*WorkerLocal[] wks = getLocalWorkers(5);
		for(int j=0;j<wks.length;j++)
			wks[j].setWorker(new WorkerMul());*/
		
		
		//让多个工人竞争处理多条消息，并且高容错
		WareHouse[] tasks = new WareHouse[15];
		for(int i=0;i<15;i++){
			tasks[i]=new WareHouse("taskId",i+"");
		}
		WareHouse[] result = doTaskCompete(wks, tasks);
		for(int i=0;i<result.length;i++){
			System.out.println(i+":"+result[i]);
		}
		
		return inhouse;
	}
	
	public static void main(String[] args)
	{
		CtorMul cd = new CtorMul();
		cd.giveTask(null);
		cd.exit();
	}
}