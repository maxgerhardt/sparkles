include <battery_casing.scad>;
include <cyl_head_bolt.scad>;
$fa = 1; $fs = $preview ? 2 : 0.5;
$fn = $preview? 20 : 200;

$ellipse_length = 105;
$ellipse_width = 55;
$base_height = 8;
$casing_straight_height = 78;
$casing_roof_height = 20;
$screw_thickness = 2;
$board_width = 43.5;
$board_thickness = 1.75;
$rail_height = 20;
$rail_width = 4.5;
$ellipse_inner_width = $ellipse_width-1.5;
$cutout_square_width = $board_width-1.5;

module elrdcyl(
   w, // width of cylinder
   d, // depth of cylinder
   h1,// straight height of cylinder
   h2 // height of rounded top
   ) {
   intersection(){
     union(){
       scale([w/2,d/2,1])cylinder(r=1,h=h1);  // cylinder
       translate([0,0,h1])scale([w/2,d/2,h2])sphere(r=1);  // top
     }
     scale([w/2,d/2,1])cylinder(r=1,h=h1+h2); // only needed if h2>h1 
   }
}


module cover_main() {
elrdcyl($ellipse_length,$ellipse_width,$casing_straight_height,$casing_roof_height);
}
$notch_height = 20;
$notch_width = 10;
$notch_start = $casing_straight_height+$casing_roof_height-3;
$through_hole = 2.5;
module notch() {
    difference() {
        difference() {
            translate([0, 0, $notch_start])
                cylinder($notch_height, d1=$notch_width+$notch_width/2, d2=$notch_width);
           cover_main();
        }
     translate([0, 0, $notch_start+$notch_height-5]) rotate([90,0,0])  cylinder($notch_width, d1=$through_hole, d2=$through_hole, center=true);
     }
 }

module cover() {

    translate([0, 0, 0]) difference() {
        cover_main();
        translate([0,0,-1]) elrdcyl($ellipse_length-1,$ellipse_width-1,$casing_straight_height, $casing_roof_height-1);
        translate([-$ellipse_length/2, 0, $base_height/2-0.5]) rotate([0,90, 0]) cylinder($ellipse_length, $screw_thickness, $screw_thickness);
    }
    notch();
}

module bottom_base() {
    difference() {
        //base
        translate([0, 100, 0]) elrdcyl($ellipse_length-1.5,$ellipse_inner_width,$base_height, 0);
        //screw
        translate([-$ellipse_length/2, 100, $base_height/2-0.5]) rotate([0,90, 0]) cylinder($ellipse_length, $screw_thickness, $screw_thickness);
        
        
    }
    //rail
    difference() {
        translate([0, 100, 10]) cube([$board_width+$rail_width, 8, $rail_height], true);
        
        //screw
        
        
    }
}

 


module bottom() {

    difference() {
        bottom_base();
        translate([0, 100, $base_height/2]) cube([$cutout_square_width,$board_width+2, $casing_straight_height], true);
    }
    difference() {
         translate([0, 100, 0]) elrdcyl($ellipse_length-1.5,$ellipse_inner_width,$base_height, 0);
        translate([0, 100, 0]) elrdcyl($ellipse_length-10,$ellipse_inner_width-10,$base_height+$rail_height, 0);
    }

}

cover();

$cutout_insert_width = 3;
$battery_to_rail_distance = 0;
$battery_cutout_width = 2;
$cutout_insert_length = ($ellipse_inner_width/2)-$screw_thickness;
module entire_bottom() {
    difference() {
            bottom();
        //one side cutout
        translate([-($board_width)/2-$rail_width/2-$battery_outer_width, 100-$battery_outer_width/2, $base_height-1]) cube([$battery_outer_width, $ellipse_inner_width, 1]);
          //left
          translate([-$board_width/2-$rail_width/2-$battery_to_rail_distance-$battery_outer_width, 100+($ellipse_inner_width)/2,  $base_height-1]) rotate([90, 270, 0]) cylinder($cutout_insert_length, $battery_cutout_width/2, $battery_cutout_width/2);
          //right
    translate([-$board_width/2-$rail_width/2-$battery_to_rail_distance, 100+($ellipse_inner_width)/2,  $base_height-1]) rotate([90, 270, 0]) cylinder($cutout_insert_length, $battery_cutout_width/2, $battery_cutout_width/2);
        
        //other side cutout
        
          translate([($board_width)/2+$rail_width/2, 100-$battery_outer_width/2, $base_height-1]) cube([$battery_outer_width, $ellipse_inner_width, 1.2]);
          //left
          translate([$board_width/2+$rail_width/2+$battery_to_rail_distance+$battery_outer_width, 100+($ellipse_inner_width)/2,  $base_height-1]) rotate([90, 270, 0]) cylinder($cutout_insert_length, $battery_cutout_width/2, $battery_cutout_width/2);
      
          //right
    translate([$board_width/2+$rail_width/2+$battery_to_rail_distance, 100+($ellipse_inner_width)/2,  $base_height-1]) rotate([90, 270, 0]) cylinder($cutout_insert_length, $battery_cutout_width/2, $battery_cutout_width/2);
      
      //board
      translate([-$board_width/2, 100-$board_thickness/2, $base_height-2]) cube([$board_width, $board_thickness, $rail_height]);
      //screws
          translate([-$ellipse_length/2, 100, $base_height/2-0.5]) rotate([180,90, 0])  hole_through(name="M3", l=12, cld=0.1, h=0, hcld=0.4);
    translate([$ellipse_length/2, 100, $base_height/2-0.5]) rotate([0,90, 0])  hole_through(name="M3", l=12, cld=0.1, h=0, hcld=0.4);
    }
}
//    entire_bottom();

//translate([-$cutout_square_width/2-$rail_width-$battery_to_rail_distance-$battery_outer_width-$battery_cutout_width/2, 100+($ellipse_inner_width)/2,  $base_height-1]) rotate([90, 270, 0]) cylinder($cutout_insert_length, $battery_cutout_width/2, $battery_cutout_width/2);




// translate([45/2+$battery_outer_width, 100-$battery_outer_width/2, 10]) rotate([0, 0, 90]) battery_casing();
// translate([-45/2, 100-$battery_outer_width/2, 10]) rotate([0, 0, 90]) battery_casing();


//color("blue", 1.0) translate([-($board_width)/2-$rail_width/2, 100-$battery_outer_width/2+$ellipse_inner_width, $base_height-1]) rotate([0,0,180]) cube([10, $ellipse_inner_width, 1]);
