import java.util.ArrayList;

import javassist.CannotCompileException;
import javassist.ClassPool;
import javassist.CtClass;
import javassist.CtMethod;
import javassist.expr.ExprEditor;
import javassist.expr.MethodCall;

public class ByteCodeTransformNew {
	
	private static String className;
	private static String destinationPath;
	//private static int thresholdValue;
	private static String classPath;
	private static final String INVALID_METHOD_NAMES="wait,equals,main,clone,getClass,finalize,notifyAll,hashCode,notify,toString";
    public static void main(String[] args)  {
    	boolean isManualOperation=false;
    	classPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\NewWorkspace\\JavaAssistTest\\newcls";
    	className="SriModified";
    	destinationPath="E:\\EMDC_IST\\SEMESTER_2\\AVExe_Virtual_Execution_Environments\\PROJECT\\NewWorkspace\\JavaAssistTest\\test";
        //thresholdValue=9;
    	if(args!=null && args.length==3 && !isManualOperation)
    	{
	    	classPath=args[0];
	    	className=args[1];
	    	destinationPath=args[2];
	    	//thresholdValue=Integer.parseInt(args[3]);
	        StartTransform();
    	}
    	else if(isManualOperation)
    	{
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
	        CtMethod method=clasRef.getDeclaredMethod("main");
	        method.insertBefore("{SriModified ss = new SriModified();}");
	        clasRef.writeFile(destinationPath);
	        clasRef.defrost();
	        method.instrument(
	        	    new ExprEditor() {
	        	        public void edit(MethodCall m)
	        	                      throws CannotCompileException
	        	        {
	        	            if (m.getClassName().equals("SriModified")
	        	                          && m.getMethodName().equals("ModifiedMethod"))
	        	                m.replace("{  $_ = $proceed($$); ss.dummy();}");
	        	        }
	        	    });
	        clasRef.writeFile(destinationPath);
	    /*    CtMethod[] validMethodArray= GetMethods(clasRef);
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
	        		CtMethod newOverloadedMethod=CtMethod.make("public void "+method.getName()+"(String message) { System.out.println(\"Threshold Hit = \"+ message); }", clasRef);
	        		clasRef.addMethod(newOverloadedMethod);
	                method.insertBefore("{ "+fieldName+"++;if("+fieldName+">threshold){System.out.println(\"Threshold Hit = \"+"+fieldName+");"+method.getName()+"(\"Exceed threshold Message\");} }");	                
	        	}
	        	clasRef.writeFile(destinationPath);
	        }
	    */
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
