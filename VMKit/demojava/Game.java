import java.util.Random;

public class Game
{

    private static int group2S_GameCalc1=0;
    private static int group2S_GameCalc2=0;
    private static int group2S_GameCalc3=0;
    private static int group2E_GameCalc1=0;
    private static int group2E_GameCalc2=0;
    private static int group2E_GameCalc3=0;
    private static int group2S_GameCalc4=0;
    private static int group2S_GameCalc5=0;
    private static int group2S_GameCalc6=0;
    private static int group2E_GameCalc4=0;
    private static int group2E_GameCalc5=0;
    private static int group2E_GameCalc6=0;
    private static int group2S_GameCalc7=0;
    private static int group2S_GameCalc8=0;
    private static int group2S_GameCalc9=0;
    private static int group2E_GameCalc7=0;
    private static int group2E_GameCalc8=0;
    private static int group2E_GameCalc9=0;
    int factorial(int n) {
        int fact = 1; // this  will be the result
        for (int i = 1; i <= n; i++) {
            fact *= i;
        }
        return fact;
    }

    public void Calc1()
    {
      ++group2S_GameCalc1;
          for(int i =1; i <500 ; ++i)
               factorial(i);  
      ++group2E_GameCalc1;
      
    }
    public void Calc2()
    {
      ++group2S_GameCalc2;
           for(int i =1; i <500 ; ++i)
               factorial(i);
      ++group2E_GameCalc2;

    }
 public void Calc3()
    {
      ++group2S_GameCalc3;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc3;
    }

 public void Calc4()
    {
      ++group2S_GameCalc4;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc4;
    }

 public void Calc5()
    {
      ++group2S_GameCalc5;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc5;
    }

 public void Calc6()
    {
      ++group2S_GameCalc6;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc6;
    }

 public void Calc7()
    {
      ++group2S_GameCalc7;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc7;
    }

 public void Calc8()
    {
      ++group2S_GameCalc8;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc8;
    }

 public void Calc9()
    {
      ++group2S_GameCalc9;
           for(int i =1; i <1000 ; ++i)
               factorial(i);

      ++group2E_GameCalc9;
    }

   
  int  group2S_GameCalc1()
    {
        return group2S_GameCalc1;
    }
  int  group2E_GameCalc1()
    {
        return group2E_GameCalc1;
    }
  int group2S_GameCalc2()
    {
        return group2S_GameCalc2;
    }
  int group2S_GameCalc3()
   {
         return group2S_GameCalc3;
   }


   int group2E_GameCalc2()
    {
        return group2E_GameCalc2;
    }
  int group2E_GameCalc3()
   {
         return group2E_GameCalc3;
   }

 int  group2S_GameCalc4()
    {
        return group2S_GameCalc4;
    }

 int  group2E_GameCalc4()
    {
        return group2E_GameCalc4;
    }
 int  group2S_GameCalc5()
    {
        return group2S_GameCalc5;
    }
 int  group2E_GameCalc5()
    {
        return group2E_GameCalc5;
    }
 int  group2S_GameCalc6()
    {
        return group2S_GameCalc6;
    }
 int  group2E_GameCalc6()
    {
        return group2E_GameCalc6;
    }
 int  group2S_GameCalc7()
    {
        return group2S_GameCalc7;
    }
 int  group2E_GameCalc7()
    {
        return group2E_GameCalc7;
    }
 int  group2S_GameCalc8()
    {
        return group2S_GameCalc8;
    }
 int  group2E_GameCalc8()
    {
        return group2E_GameCalc8;
    }
 int  group2S_GameCalc9()
    {
        return group2S_GameCalc9;
    }
 int  group2E_GameCalc9()
    {
        return group2E_GameCalc9;
    }
    public static void main(String[] args)
   {

      Game gg = new Game();
      Random rr = new Random();
      gg.group2S_GameCalc1();
      gg.group2S_GameCalc2();
      gg.group2S_GameCalc3();
      gg.group2E_GameCalc1();
      gg.group2E_GameCalc2();
      gg.group2E_GameCalc3();
      gg.group2S_GameCalc4();
      gg.group2S_GameCalc5();
      gg.group2S_GameCalc6();
      gg.group2E_GameCalc4();
      gg.group2E_GameCalc5();
      gg.group2E_GameCalc6();
      gg.group2S_GameCalc7();
      gg.group2S_GameCalc8();
      gg.group2S_GameCalc9();
      gg.group2E_GameCalc7();
      gg.group2E_GameCalc8();
      gg.group2E_GameCalc9();
    for(int j =0; j <100 ; ++j)
     {
      gg.Calc1();
      gg.Calc2();
     }

    System.out.println("****************************************************");
  for(int j =0; j <5000 ; ++j)
     {
      gg.Calc3(); 

       if( j >200)
          {
       gg.Calc1();
       gg.Calc2();

         }
///         try 
   //         {
     //       int sleeptime =  rr.nextInt(100);
       //     Thread.sleep((long)(sleeptime));
         //   }
           // catch (Exception e)
           //{

          // }
     }
  for(int j =0; j <10 ; ++j)
     {
      gg.Calc1();
   
     }

  for(int j =0; j <10 ; ++j)
     {
      gg.Calc2();
     }
for(int j =0; j <4000 ; ++j)
     {
      gg.Calc4();
      gg.Calc5();
      gg.Calc6();
   
     }

  for(int j =0; j <3000 ; ++j)
     {
      gg.Calc4();
      gg.Calc5();
      gg.Calc6();
      gg.Calc7();
      gg.Calc8();
      gg.Calc9();
     }

   }

}
