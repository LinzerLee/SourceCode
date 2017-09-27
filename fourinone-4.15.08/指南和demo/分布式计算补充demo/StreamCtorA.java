import com.fourinone.Contractor;
import com.fourinone.WareHouse;
import com.fourinone.WorkerLocal;
import java.util.ArrayList;

public class StreamCtorA extends Contractor
{
	public WareHouse giveTask(WareHouse inhouse)
	{
		WorkerLocal[] wks = getWaitingWorkers("StreamWorkerA");
		System.out.println("wks.length:"+wks.length);
		
		WareHouse result = wks[0].doTask(inhouse);
		while(true){
			if(result.getStatus()!=WareHouse.NOTREADY)
			{
				break;
			}
		}
		return result;
		
		//WareHouse[] result = doTaskBatch(wks, inhouse);
		//return result[0];
	}
	
	public static void main(String[] args)
	{
		StreamCtorA sc = new StreamCtorA();
		for(int i=0;i<10;i++){
			WareHouse msg = new WareHouse("msg","hello"+i);
			WareHouse wh = sc.giveTask(msg);
			System.out.println(wh);
		}
		sc.exit();
	}
}