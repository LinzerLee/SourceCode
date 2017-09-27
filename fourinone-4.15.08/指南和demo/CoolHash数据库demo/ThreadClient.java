import com.fourinone.BeanContext;
import com.fourinone.CoolHashClient;

public class ThreadClient implements Runnable{
	public CoolHashClient chc;
	
	public ThreadClient(CoolHashClient chc){
		this.chc=chc;
	}
	
	//����߳������޸�threadname, ���Լ��߳�������Ϊ��value�������ɵ�valueֵ���
	public void putTest(){
		try{
			long begintime = System.currentTimeMillis();
            String threadname = (String)chc.put("threadname",Thread.currentThread().getName());
           	System.out.println(Thread.currentThread().getName()+" put time:"+(System.currentTimeMillis()-begintime)+" old:"+threadname);
           	
           	//chc.exit();//���̲߳���δ������Ҫ�رտͻ���
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	//д������У�ͬʱ�ж���߳�������ȡthreadname�����
	public void getTest(){
		try{
			long begintime = System.currentTimeMillis();
            String threadname = (String)chc.get("threadname");
           	System.out.println(Thread.currentThread().getName()+" get time:"+(System.currentTimeMillis()-begintime)+" get:"+threadname);
           	
           	//chc.exit();//���̲߳���δ������Ҫ�رտͻ���
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	public void run(){
		putTest();//�߳���д��
		getTest();//�߳��ж�ȡ
	}
	
	//�������java -cp fourinone.jar; ThreadPutGet localhost 2014 1000
	//Ϊ�������鿴���߳������������Թر�һЩϵͳ��������ͻ��˵�config.xml��LOG���ֵ�DESC="LOGLEVEL"���õ�OFF����
	public static void main(String[] args){
		String host = args[0];
		int ip = Integer.parseInt(args[1]);
		int threadcount=Integer.parseInt(args[2]);
		
		for(int i=0;i<threadcount;i++){
			CoolHashClient chc = BeanContext.getCoolHashClient(host,ip);
			ThreadClient tc = new ThreadClient(chc);
			new Thread(tc).start();
        }
	}
}