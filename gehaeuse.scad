$fa = 1; $fs = $preview ? 2 : 0.5;
//$fn=200;

$move = 35;

$bottom_cylinder_inner = 40;
$carrier_height = 16;
$carrier_width = 8;
$pcb_width = 1.77;
$carrier_length = 50;
$bottom_height=10;
$pcb_width=41.5;
$pcb_height = 65;
$middle_height=$pcb_height+3;
$bottom_cylinder_outer = $carrier_length+12;
$notch_height = 20;
$notch_width = 10;
$top_height = 20;





module cylbottom() {
    difference() {
        cylinder(h=$bottom_height, d1=$bottom_cylinder_outer, d2=$bottom_cylinder_outer);
        cylinder(h=$bottom_height, d1=$bottom_cylinder_inner, d2=$bottom_cylinder_inner);
    }
}
module cyltop_outer() {
cylinder(h=$top_height, d1=$bottom_cylinder_outer-8, d2=0);
}
module cyltop() {
    translate([0, 0, $bottom_height+$middle_height]) difference() {
        cyltop_outer();
        cylinder(h=$top_height-2, d1=$bottom_cylinder_outer-8.8, d2=0);
    }
}

module notch() {
    difference() {
        translate([0, 0, $bottom_height+$middle_height+$top_height-5])
            cylinder($notch_height, d1=$notch_width, d2=$notch_width);
        cyltop_outer();
    }
}

module cylmiddle() {
    translate([0, 0, $bottom_height]) 
        difference() {
        cylinder($middle_height, d1=$bottom_cylinder_outer, d2=$bottom_cylinder_outer-8);
         cylinder($middle_height, d1=$bottom_cylinder_outer-0.8, d2=$bottom_cylinder_outer-8.8);
        }
}

//translate([0,0,5]) cyltop();

module cylinder_bottom() {    
    difference() {
        cylbottom();
        translate([0,0,$bottom_height/2+1]) cube([$carrier_width+1,$carrier_length+2,$bottom_height], true);
        rotate([0, 0,90])cube([$carrier_width+12, $carrier_length+4, $bottom_height*2], true);
    }
}
module carrier_negative() {
        translate([$move,0,4+$carrier_height/2]) color([0,0,1]) cube([$carrier_width,37,4+$carrier_height/2], true);
        translate([$move,0.5,8+$carrier_height/2]) color([0,1,0]) cube([$carrier_width,37.5,4+$carrier_height/2], true);
        translate([$move,-0.5,3+$carrier_height/2]) color([1,0,0]) cube([$pcb_width,$pcb_width,6+$carrier_height/2], true);
        translate([$move,-2,0+$carrier_height/2]) color(0,1,0) cube([3.5, 20,+$carrier_height/2], true);
    translate([$move+$carrier_width/2+$pcb_width/2,-16,$carrier_height-5]) color(0,1,0) cube([$carrier_width, 8, 12], true);
    }

    

module carrier() { 
    difference() {
        translate([$move,0,$carrier_height/2]) color([0,0,1]) cube([$carrier_width,$carrier_length,$carrier_height], true);
    carrier_negative();
    }

}
//carrier();
//cylinder_bottom();
//cylmiddle();
//cyltop();
notch();

