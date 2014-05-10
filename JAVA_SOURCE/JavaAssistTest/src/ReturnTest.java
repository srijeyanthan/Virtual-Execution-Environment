import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;


public class ReturnTest 
{
	
	private static String className;
	private static String destinationPath;
	private static String classPath;
	//private static final String INVALID_METHOD_NAMES="wait,equals,main,clone,getClass,finalize,notifyAll,hashCode,notify,toString";
    public static void main(String[] args)  
    {    	
    	classPath="E:\\BYTECODETEST\\RETURN\\s";
    	className="test";
    	destinationPath="E:\\BYTECODETEST\\RETURN\\d";
    	try
    	{
	    	ClassPool classPool = ClassPool.getDefault();
	    	classPool.insertClassPath(classPath);
	        CtClass clasRef = classPool.get(className);
	        CtMethod method=clasRef.getDeclaredMethod("getCount");
	        method.insertAfter("{System.out.println(\"Yes\");}");
	        clasRef.writeFile(destinationPath);
	        System.out.println("Done");
    	}
    	catch(Exception ex)
    	{
    		ex.printStackTrace();
    	}
    }

}
