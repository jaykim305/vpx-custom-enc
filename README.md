# vpx-custom-enc



# testing videos  
```
cd script
bash ffmpeg_test.sh [content] #make using ffmpeg  
bash vpx_test.sh [content] #make using vpx   
bash compare_psnr.sh [content] #get psnr results  
```
# results
- Hearthstone (1080p, 60fps)
  - **my_vpxenc**  
  PSNR y:39.433927 u:42.226109 v:43.415964 **average:40.290927** min:32.684165 max:47.098691  
  200fps  
  - **ffmpeg**  
  PSNR y:39.494910 u:42.304841 v:43.497325 **average:40.355452** min:32.424708 max:50.370501  


- CSGO (1080p, 60fps)
  - **my_vpxenc**  
  PSNR y:32.192989 u:40.650755 v:42.266148 **average:33.699856** min:30.047399 max:55.728286  
  121fps  
  - **ffmpeg**   
  PSNR y:32.321720 u:40.661769 v:42.278743 **average:33.821850** min:30.023484 max:55.747964
