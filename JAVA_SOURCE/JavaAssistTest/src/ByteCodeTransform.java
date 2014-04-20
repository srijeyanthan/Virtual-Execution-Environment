import java.util.ArrayList;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtField;
import javassist.CtMethod;

public class ByteCodeTransform {
	
	private static String className;
	private static String destinationPath;
	private static int thresholdValue;
	private static String classPath;
	private static final String INVALID_METHOD_NAMES="wait,equals,main,clone,getClass,finalize,notifyAll,hashCode,notify,toString";
    public static void main(String[] args)  {
    	//classPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\Workspace\\JavaAssistTest\\newcls";
    	//className="Hello";
    	//destinationPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\Workspace\\JavaAssistTest\\test";
        //thresholdValue=9;
    	if(args!=null && args.length==4)
    	{
	    	classPath=args[0];
	    	className=args[1];
	    	destinationPath=args[2];
	    	thresholdValue=Integer.parseInt(args[3]);
	        StartTransform();
    	}
    	else
    	{
    		System.err.println("Please pass argumets\n[0]-classPath\n[1]-className\n[2]-destinationPath\n[3]-thresholdValue");
    	}
    }
    
    public static void StartTransform()
    {
    	try
    	{
	    	ClassPool classPool = ClassPool.getDefault();
	    	classPool.insertClassPath(classPath);
	        CtClass clasRef = classPool.get(className);
	        CtMethod[] validMethodArray= GetMethods(clasRef);
	        CtField field=null;
	        if(validMethodArray!=null)
	        {
	        	String fieldName;
	        	field=CtField.make("private int threshold="+thresholdValue+";", clasRef);
	        	clasRef.addField(field);
	        	for(CtMethod method:validMethodArray)
	        	{
	        		fieldName=method.getName()+"Count";
	        		field=CtField.make("private int "+fieldName+"=0;", clasRef);
	        		clasRef.addField(field);	        		                
	                method.insertBefore("{ "+fieldName+"++;if("+fieldName+">threshold){System.out.println(\"Threshold Hit = \"+"+fieldName+");} }");	                
	        	}
	        	clasRef.writeFile(destinationPath);
	        }
    	}
    	catch (Exception e) 
    	{
			System.err.println(e.getMessage());
		}
    }
    
    public static boolean IsValidateMethod(String methodName)
    {    	
    	boolean isValid=true;
    	String[] invalidMethodNameList=INVALID_METHOD_NAMES.split(",");
    	for(int i=0;i<invalidMethodNameList.length;i++)
    	{
    		if(methodName.equals(invalidMethodNameList[i]))
    		{
    			isValid=false;
    			break;
    		}
    	}
    	return isValid;
    }
    
    public static CtMethod[] GetMethods(CtClass classRef)
    {
    	ArrayList<CtMethod> methodsInterested =new ArrayList<CtMethod>();
    	CtMethod[] list=classRef.getMethods();
    	if(list!=null && list.length>0)
    	{
    		for(int i=0;i<list.length;i++)
    		{
    			if(IsValidateMethod(list[i].getName()))
    				methodsInterested.add(list[i]);
    		}    		
    	}
    	return (methodsInterested.size()>0)?methodsInterested.toArray(new CtMethod[methodsInterested.size()]):null;
    }
}
