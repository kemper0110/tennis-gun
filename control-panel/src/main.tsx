import '@ant-design/v5-patch-for-react-19';
import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import AppLoader from './AppLoader.tsx'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <AppLoader />
  </StrictMode>,
)
